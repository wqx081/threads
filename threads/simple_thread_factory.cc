#include "threads/simple_thread_factory.h"
#include <assert.h>
#include <pthread.h>

namespace threads {

class SimpleThread : public Thread {
 public:
  enum StateEnum { UNINITIALIZED, STARTING, STARTED, STOPPING, STOPPED };
  static const int kMB = 1024 * 1024;

  static void* ThreadMain(void* arg);

  SimpleThread(int policy, 
               int priority, 
               int stack_size, 
               bool detached, 
               std::shared_ptr<Runnable> runnable)
    : pthread_(0),
      state_(UNINITIALIZED),
      policy_(policy),
      priority_(priority),
      stack_size_(stack_size),
      detached_(detached) {
    this->Thread::SetRunnable(runnable);
  }

  ~SimpleThread() {
    if (!detached_) {
      try {
        Join();
      } catch (...) {
      }  
    }
  }

  void Start() override {
    if (state_ != UNINITIALIZED) {
      return;
    }
    pthread_attr_t thread_attr;
    if (pthread_attr_init(&thread_attr) != 0) {
    }
    if (pthread_attr_setdetachstate(&thread_attr,
                                    detached_ ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE) != 0){
    }
    if (pthread_attr_setstacksize(&thread_attr, kMB * stack_size_) != 0) {
    }

    if (pthread_attr_setschedpolicy(&thread_attr, policy_) != 0) {
    }
    sched_param sched_param;
    sched_param.sched_priority = priority_;

    if (pthread_attr_setschedparam(&thread_attr, &sched_param) != 0) {
    }

    std::shared_ptr<SimpleThread>* self_ref = new std::shared_ptr<SimpleThread>();
    *self_ref = self_.lock();

    state_ = STARTING;

    if (pthread_create(&pthread_, &thread_attr, ThreadMain, (void*)self_ref) != 0) {
    }
  }

  void Join() override {
    if (!detached_ && state_ != UNINITIALIZED) {
      void* ignore;
      int res = pthread_join(pthread_, &ignore);
      detached_ = (res == 0);
    }
  }

  Thread::id_t GetId() override {
    return (Thread::id_t) pthread_;
  }

  std::shared_ptr<Runnable> GetRunnable() const override {
    return Thread::GetRunnable();
  }

  void SetRunnable(std::shared_ptr<Runnable> value) override {
    Thread::SetRunnable(value);
  }

  void WeakRef(std::shared_ptr<SimpleThread> self) {
    assert(self.get() == this);
    self_ = std::weak_ptr<SimpleThread>(self);
  }

 private:
  pthread_t pthread_;
  StateEnum state_;
  int policy_;
  int priority_;
  int stack_size_;
  std::weak_ptr<SimpleThread> self_;
  bool detached_;
};

void* SimpleThread::ThreadMain(void* arg) {
  std::shared_ptr<SimpleThread> thread = *(std::shared_ptr<SimpleThread>*)arg;
  delete reinterpret_cast<std::shared_ptr<SimpleThread>*>(arg);

  if (thread == nullptr) {
    return nullptr;
  }
  if (thread->state_ != STARTING) {
    return nullptr;
  }
  thread->state_ = STARTED;
  thread->GetRunnable()->Run();
  if (thread->state_ != STOPPING && thread->state_ != STOPPED) {
    thread->state_ = STOPPING;
  }
  return nullptr;
}

class SimpleThreadFactory::Impl {
 public:
  Impl(PolicyEnum policy,
       PriorityEnum priority,
       int stack_size,
       bool detached)
    : policy_(policy),
      priority_(priority),
      stack_size_(stack_size),
      detached_(detached) {}

  std::shared_ptr<Thread> NewThread(std::shared_ptr<Runnable> runnable) const {
    std::shared_ptr<SimpleThread> result = std::shared_ptr<SimpleThread>(new SimpleThread(
      ToNativePolicy(policy_),
      ToNativePriority(policy_, priority_),
      stack_size_,
      detached_,
      runnable));
    result->WeakRef(result);
    runnable->SetThread(result);
    return result;
  }

  int GetStackSize() const {
    return stack_size_;
  }
  void SetStackSize(int value) {
    stack_size_ = value;
  }

  PriorityEnum GetPriority() const {
    return priority_;
  }

  void SetPriority(PriorityEnum priority) {
    priority_ = priority;
  }
  
  bool IsDetached() const {
    return detached_;
  }

  void SetDetached(bool value) {
    detached_ = value;
  }

  Thread::id_t GetCurrentThreadId() const {
    return (Thread::id_t)pthread_self();
  }
  
 private:
  PolicyEnum policy_;
  PriorityEnum priority_;
  int stack_size_;
  bool detached_;

  static int ToNativePolicy(PolicyEnum policy) {
    switch (policy) {
      case OTHER: return SCHED_OTHER;
      case FIFO: return SCHED_FIFO;
      case ROUND_ROBIN: return SCHED_RR;
    }
    return SCHED_OTHER;
  }                                                                          

  static int ToNativePriority(PolicyEnum policy, PriorityEnum priority) {
    int pthread_policy = ToNativePolicy(policy);
    int min_priority = 0;
    int max_priority = 0;
    min_priority = sched_get_priority_min(pthread_policy);
    max_priority = sched_get_priority_max(pthread_policy);
    int quanta = (HIGHEST - LOWEST) + 1;
    float stepsper_quanta = (float)(max_priority - min_priority) / quanta;
    if (priority <= HIGHEST) {
      return (int)(min_priority + stepsper_quanta * priority);
    } else {
      assert(false);
      return (int)(min_priority + stepsper_quanta * NORMAL);
    }
    assert(false);
    return (int)(min_priority + stepsper_quanta * NORMAL);
  }

};


SimpleThreadFactory::SimpleThreadFactory(PolicyEnum policy,
                                         PriorityEnum priority,
                                         int stack_size,
                                         bool detached) 
  : impl_(new SimpleThreadFactory::Impl(policy, priority, stack_size, detached)) {}

std::shared_ptr<Thread> SimpleThreadFactory::NewThread(std::shared_ptr<Runnable> runnable) const {
  return impl_->NewThread(runnable);
}

int SimpleThreadFactory::GetStackSize() const {
  return impl_->GetStackSize();
}

void SimpleThreadFactory::SetStackSize(int value) {
  impl_->SetStackSize(value);
}

SimpleThreadFactory::PriorityEnum SimpleThreadFactory::GetPriority() const {
  return impl_->GetPriority();
}

void SimpleThreadFactory::SetPriority(PriorityEnum value) {
  impl_->SetPriority(value);
}

bool SimpleThreadFactory::IsDetached() const {
  return impl_->IsDetached();
}

void SimpleThreadFactory::SetDetached(bool value) {
  impl_->SetDetached(value);
}

Thread::id_t SimpleThreadFactory::GetCurrentThreadId() const {
  return impl_->GetCurrentThreadId();
}

} // namespace threads
