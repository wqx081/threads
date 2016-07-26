#ifndef THREADS_THREAD_MANAGER_H_
#define THREADS_THREAD_MANAGER_H_
#include "base/macros.h"
#include "threads/thread.h"

#include <memory>
#include <functional>
#include <sys/types.h>

namespace threads {

class ThreadManager {
 protected:
  ThreadManager() {}

 public:
  typedef std::function<void(std::shared_ptr<Runnable>)> ExpireCallback;

  virtual ~ThreadManager();
//  virtual ~ThreadManager() {}
  
  enum STATE {
    UNINITIALIZED, 
    STARTING, 
    STARTED,
    JOINING,
    STOPPING,
    STOPPED,
  };

  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual void Join() = 0;
 
  virtual STATE GetState() const = 0;
  virtual std::shared_ptr<ThreadFactory> GetThreadFactory() const = 0;
  virtual void SetThreadFactory(std::shared_ptr<ThreadFactory> value) = 0;

  virtual void AddWorker(size_t value=1) = 0; 
  virtual void RemoveWorker(size_t value=1) = 0;

  virtual size_t IdleWorkerCount() const = 0;
  virtual size_t WorkerCount() const = 0;

  virtual size_t PendingTaskCount() const = 0;
  virtual size_t TotalTaskCount() const = 0;
  virtual size_t PendingTaskCountMax() const = 0;
  virtual size_t ExpiredTaskCount() = 0;

  virtual void Add(std::shared_ptr<Runnable> task,
                   int64_t timeout = 0LL,
                   int64_t expiration = 0LL) = 0;
  virtual void Remove(std::shared_ptr<Runnable> task) = 0;
  virtual std::shared_ptr<Runnable> RemoveNextPending() = 0;
  virtual void RemoveExpiredTasks() = 0;

  virtual void SetExpireCallback(ExpireCallback expire_callback) = 0;

  static std::shared_ptr<ThreadManager> NewThreadManager();
  static std::shared_ptr<ThreadManager> NewSimpleThreadManager(size_t count = 4,
                                                               size_t pending_task_count_max = 0);
  //public
  class Task;
  class Worker;
  class Impl;
};

} // namespace threads
#endif // THREADS_THREAD_MANAGER_H_
