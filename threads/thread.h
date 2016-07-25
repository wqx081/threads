#ifndef THREADS_THREAD_H_
#define THREADS_THREAD_H_
#include <stdint.h>
#include <memory>

#include <pthread.h>

namespace threads {

class Thread;

class Runnable {
 public:
  virtual ~Runnable() {}
  virtual void Run() = 0;
  virtual std::shared_ptr<Thread> GetThread() { return thread_.lock(); }
  virtual void SetThread(std::shared_ptr<Thread> thread) { thread_ = thread; } 
 private:
  std::weak_ptr<Thread> thread_;
};

class Thread {
 public:
  typedef pthread_t id_t;

  static inline bool IsCurrent(id_t t) {
    return pthread_equal(pthread_self(), t);
  }

  static inline id_t GetCurrent() {
    return pthread_self();
  }

  virtual ~Thread() {};

  virtual void Start() = 0;
  virtual void Join() = 0;
  virtual id_t GetId() = 0;
  virtual std::shared_ptr<Runnable> GetRunnable() const {
    return runnable_;
  }

 protected:
  virtual void SetRunnable(std::shared_ptr<Runnable> value) {
    runnable_ = value;
  }

 private:
  std::shared_ptr<Runnable> runnable_;
};

class ThreadFactory {
 public:
  virtual ~ThreadFactory() {}
  virtual bool IsDetached() const = 0;
  virtual std::shared_ptr<Thread> NewThread(std::shared_ptr<Runnable> runnable) const = 0;
  virtual void SetDetached(bool detached) = 0;

  static const Thread::id_t unknown_thread_id;

  virtual Thread::id_t GetCurrentThreadId() const = 0;
};

} // namespace threads
#endif // THREADS_THREAD_H_
