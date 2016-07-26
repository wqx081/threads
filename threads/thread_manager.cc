#include "threads/thread_manager.h"
#include "threads/monitor.h"
#include "threads/exception.h"

#include "base/time_util.h"

#include <assert.h>
#include <queue>
#include <set>
#include <map>

namespace threads {

ThreadManager::~ThreadManager() {
}

class ThreadManager::Impl : public ThreadManager {
 public:
  Impl()
    : worker_count_(0),
      worker_max_count_(0),
      idle_count_(0),
      pending_task_count_max_(0),
      expired_count_(0),
      state_(ThreadManager::UNINITIALIZED),
      monitor_(&mutex_),
      max_monitor_(&mutex_) {}

  ~Impl() { Stop(); }

  void Start() override;

  void Stop() override {
    StopImpl(false);
  }

  void Join() override {
    StopImpl(true);
  }

  ThreadManager::STATE GetState() const override {
    return state_;
  }

  std::shared_ptr<ThreadFactory> GetThreadFactory() const override {
    Synchronized s(monitor_);
    return thread_factory_;
  }

  void SetThreadFactory(std::shared_ptr<ThreadFactory> value) override {
    Synchronized s(monitor_);
    thread_factory_ = value;
  }

  void AddWorker(size_t value) override;
  void RemoveWorker(size_t value) override;

  size_t IdleWorkerCount() const override {
    // Synchronized???
    return idle_count_;
  }
  size_t WorkerCount() const override {
    Synchronized s(monitor_);
    return worker_count_;
  } 

  size_t PendingTaskCount() const override {
    Synchronized s(monitor_);
    return tasks_.size();
  }
  size_t TotalTaskCount() const override {
    Synchronized s(monitor_);
    return tasks_.size() + worker_count_ - idle_count_;
  }
  size_t PendingTaskCountMax() const override {
    Synchronized s(monitor_);
    return pending_task_count_max_;
  }
  size_t ExpiredTaskCount() override {
    Synchronized s(monitor_);
    size_t result = expired_count_;
    expired_count_ = 0;
    return result;
  }

  void Add(std::shared_ptr<Runnable> value, int64_t timeout, int64_t expiration) override;
  void Remove(std::shared_ptr<Runnable> task) override;
  std::shared_ptr<Runnable> RemoveNextPending() override;
  void RemoveExpiredTasks() override;
  void SetExpireCallback(ExpireCallback expire_callback) override;

  void SetPendingTaskCountMax(const size_t value) {
    Synchronized s(monitor_);
    pending_task_count_max_ = value;
  }
  bool CanSleep();
  
 private:
  size_t worker_count_;
  size_t worker_max_count_;
  size_t idle_count_;
  size_t pending_task_count_max_;
  size_t expired_count_;
  ExpireCallback expire_callback_;

  ThreadManager::STATE state_;
  std::shared_ptr<ThreadFactory> thread_factory_;

  friend class ThreadManager::Task;
  std::queue<std::shared_ptr<Task>> tasks_;
  Mutex mutex_;
  Monitor monitor_;
  Monitor max_monitor_;
  Monitor worker_monitor_;

  friend class ThreadManager::Worker;
  std::set<std::shared_ptr<Thread>> workers_;
  std::set<std::shared_ptr<Thread>> dead_workers_;
  std::map<const Thread::id_t, std::shared_ptr<Thread>> id_map_;

  void StopImpl(bool join);
};

class ThreadManager::Task : public Runnable {
 public:
  enum STATE { WAITING, EXECUTING, CANCELLED, COMPLETE };
  Task(std::shared_ptr<Runnable> runnable,
       int64_t expiration = 0LL)
    : runnable_(runnable),
      state_(WAITING),
      expire_time_(expiration != 0LL ? base::TimeUtil::CurrentTime() + expiration : 0LL) {}

  ~Task() {}

  void Run() override {
    if (state_ == EXECUTING) {
      runnable_->Run();
      state_ = COMPLETE;
    }
  }

  std::shared_ptr<Runnable> GetRunnable() { 
    return runnable_;                                           
  }
  int64_t GetExpireTime() const {
    return expire_time_;
  }

 private:
  std::shared_ptr<Runnable> runnable_;  
  friend class ThreadManager::Worker;
  STATE state_;
  int64_t expire_time_;
};

class ThreadManager::Worker : public Runnable {
 private:
  enum STATE { UNINITIALIZED, STARTING, STARTED, STOPPING, STOPPED };

 public:

  Worker(ThreadManager::Impl* manager)
    : manager_(manager), state_(UNINITIALIZED), idle_(false) {
  }

  ~Worker() {}
  
  void Run() override {
    bool active = false;

    {
      bool notify_manager = false;
      {
        Synchronized s(manager_->monitor_);
        active = manager_->worker_count_ < manager_->worker_max_count_;
        if (active) {
          manager_->worker_count_++;
          notify_manager = manager_->worker_count_ == manager_->worker_max_count_;
        }
      }
      if (notify_manager) {
        Synchronized s(manager_->worker_monitor_);
        manager_->worker_monitor_.Notify();
      }
    }

    while (active) {
      std::shared_ptr<ThreadManager::Task> task;
      {
        Guard g(manager_->mutex_);
        active = IsActive();

        while (active && manager_->tasks_.empty()) {
          manager_->idle_count_++;
          idle_ = true;
          manager_->monitor_.Wait();
          active = IsActive(); idle_ = false;
          manager_->idle_count_--; 
        }

        if (active) {
          manager_->RemoveExpiredTasks();

          if (!manager_->tasks_.empty()) {
            task = manager_->tasks_.front();
            manager_->tasks_.pop();
            if (task->state_ == ThreadManager::Task::WAITING) {
              task->state_ = ThreadManager::Task::EXECUTING;
            }
          }

          if (manager_->pending_task_count_max_ != 0 &&
              manager_->tasks_.size() <= manager_->pending_task_count_max_ - 1) {
            manager_->max_monitor_.Notify();
          }
        }
      }
     
      if (task) {
        if (task->state_ == ThreadManager::Task::EXECUTING) {
          try {
            task->Run();
          } catch (...) {
            //LOG(ERROR) << "task->Run() raised and exception: ";
          }
        }
      }
    }

    {
      Synchronized s(manager_->worker_monitor_);
      manager_->dead_workers_.insert(this->GetThread());
      idle_ = true;
      manager_->worker_count_--;
      bool notify_manager = (manager_->worker_count_ == manager_->worker_max_count_);
      if (notify_manager) {
        manager_->worker_monitor_.Notify();
      }
    }
    return;
  }

 private:
  ThreadManager::Impl* manager_;
  friend class ThreadManager::Impl;
  STATE state_;
  bool idle_;

  bool IsActive() const {
    return (manager_->worker_count_ <= manager_->worker_max_count_)
        || (manager_->state_ == JOINING && !manager_->tasks_.empty());
  }

};

void ThreadManager::Impl::AddWorker(size_t value) {
  std::set<std::shared_ptr<Thread>> new_threads;

  for (size_t ix = 0; ix < value; ++ix) {
    std::shared_ptr<ThreadManager::Worker> worker = std::make_shared<ThreadManager::Worker>(this);
    new_threads.insert(thread_factory_->NewThread(worker));
  }

  {
    Synchronized s(monitor_);
    worker_max_count_ += value;
    workers_.insert(new_threads.begin(), new_threads.end());
  }
  
  for (std::set<std::shared_ptr<Thread>>::iterator it = new_threads.begin();
       it != new_threads.end();
       ++it) {
    std::shared_ptr<ThreadManager::Worker> worker =
      std::dynamic_pointer_cast<ThreadManager::Worker, Runnable>((*it)->GetRunnable());
    worker->state_ = ThreadManager::Worker::STARTING;
    (*it)->Start();
    id_map_.insert(std::pair<const Thread::id_t, std::shared_ptr<Thread>>((*it)->GetId(), *it));
  }  

  {
    Synchronized s(worker_monitor_);
    while (worker_count_ != worker_max_count_) {
      worker_monitor_.Wait();
    }
  }
}

void ThreadManager::Impl::Start() {
  if (state_ == ThreadManager::STOPPED) {
    return;
  }
  {
    Synchronized s(monitor_);
    if (state_ == ThreadManager::UNINITIALIZED) {
      if (!thread_factory_) {
        throw InvalidArgumentException();
      }
      state_ = ThreadManager::STARTED;
      monitor_.NotifyAll();
    }

    while (state_ == STARTING) {
      monitor_.Wait();
    }
  }
}

void ThreadManager::Impl::StopImpl(bool join) {
  bool do_stop = false;
  if (state_ == ThreadManager::STOPPED) {
    return;
  }

  {
    Synchronized s(monitor_);
    if (state_ != ThreadManager::STOPPING && state_ != ThreadManager::JOINING
        && state_ != ThreadManager::STOPPED) {
      do_stop = true;
      state_ = join ? ThreadManager::JOINING : ThreadManager::STOPPING;
    }
  }

  if (do_stop) {
    RemoveWorker(worker_count_);
  }

  {
    Synchronized s(monitor_);
    state_ = ThreadManager::STOPPED;
  }
}

void ThreadManager::Impl::RemoveWorker(size_t value) {

  std::set<std::shared_ptr<Thread>> removed_threads;
  {
    Synchronized s(monitor_);
    if (value > worker_max_count_) {
      throw InvalidArgumentException();
    }

    worker_max_count_ -= value;

    if (idle_count_ < value) {
      for (size_t ix = 0; ix < idle_count_; ++ix) {
        monitor_.Notify();
      }  
    } else {
      monitor_.NotifyAll();
    }
  }

  {
    Synchronized s(worker_monitor_);
    
    while (worker_count_ != worker_max_count_) {
      worker_monitor_.Wait();
    }

    for (std::set<std::shared_ptr<Thread>>::iterator it = dead_workers_.begin();
         it != dead_workers_.end();
         ++it) {
      id_map_.erase((*it)->GetId());
      workers_.erase(*it);
    }

    dead_workers_.clear();

  }
}

bool ThreadManager::Impl::CanSleep() {
  const Thread::id_t id = thread_factory_->GetCurrentThreadId();
  return id_map_.find(id) == id_map_.end();
}

void ThreadManager::Impl::Add(std::shared_ptr<Runnable> value, int64_t timeout, int64_t expiration) {

  Guard g(mutex_, timeout);  

  if (!g) {
    throw TimedOutException();
  }

  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException("ThreadManager::Impl::Add ThreadManager not started");    
  }

  RemoveExpiredTasks();
  if (pending_task_count_max_ > 0 &&
      (tasks_.size() >= pending_task_count_max_)) {
    if (CanSleep() && timeout >= 0) {
      while (pending_task_count_max_ > 0 && tasks_.size() >= pending_task_count_max_) {
        max_monitor_.Wait(timeout);
      }
    } else {
      throw TooManyPendingTasksException();
    }
  }

  tasks_.push(std::make_shared<ThreadManager::Task>(value, expiration));

  if (idle_count_ > 0) {
    monitor_.Notify();
  }
}

void ThreadManager::Impl::Remove(std::shared_ptr<Runnable> task) {
  (void)task;
  Synchronized s(monitor_);
  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException("ThreadManager::Impl::Remove ThreadManager not started");
  }
}

std::shared_ptr<Runnable> ThreadManager::Impl::RemoveNextPending() {
  Guard g(mutex_);
  if (state_ == ThreadManager::STARTED) {
    throw IllegalStateException("ThreadManager::Impl::RemoveNextPending ThreadManager not started");
  }
  
  if (tasks_.empty()) {
    //return std::make_shared<Runnable>();
    return nullptr;
  }

  std::shared_ptr<ThreadManager::Task> task = tasks_.front();
  tasks_.pop();  

  return task->GetRunnable();
}

void ThreadManager::Impl::RemoveExpiredTasks() {
  int64_t now = 0LL;
  
  while (!tasks_.empty()) {
    std::shared_ptr<ThreadManager::Task> task = tasks_.front();
    if (task->GetExpireTime() == 0LL) {
      break;
    }
    if (now == 0LL) {
      now = base::TimeUtil::CurrentTime();
    }

    if (task->GetExpireTime() > now) {
      break;
    }

    if (expire_callback_) {
      expire_callback_(task->GetRunnable());
    }

    tasks_.pop();
    expired_count_++;
  }
}

void ThreadManager::Impl::SetExpireCallback(ExpireCallback expire_callback) {
  expire_callback_ = expire_callback;
}


///////////////////////
class SimpleThreadManager : public ThreadManager::Impl {
 public:
  SimpleThreadManager(size_t worker_count = 4, size_t pending_task_count_max = 0)
      : worker_count_(worker_count), pending_task_count_max_(pending_task_count_max) {
  }

  void Start() {
    ThreadManager::Impl::SetPendingTaskCountMax(pending_task_count_max_);
    ThreadManager::Impl::Start();
    AddWorker(worker_count_);
  }

 private:
  const size_t worker_count_;
  const size_t pending_task_count_max_;
  Monitor monitor_;
};

std::shared_ptr<ThreadManager> ThreadManager::NewThreadManager() {
  return std::shared_ptr<ThreadManager>(new ThreadManager::Impl());
}

std::shared_ptr<ThreadManager> ThreadManager::NewSimpleThreadManager(size_t count,
                                                                     size_t pending_task_count_max) {
  return std::shared_ptr<ThreadManager>(new SimpleThreadManager(count, pending_task_count_max));
}


} // namespace threads
