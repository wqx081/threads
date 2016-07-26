#ifndef THREADS_MONITOR_H_
#define THREADS_MONITOR_H_
#include "base/macros.h"
#include "threads/mutex.h"

namespace threads {

class Monitor {
 public:
  Monitor();
  explicit Monitor(Mutex*);
  explicit Monitor(Monitor*);

  virtual ~Monitor();

  Mutex& GetMutex() const;
  virtual void Lock() const;
  virtual void Unlock() const;

  int WaitForTimeRelative(int64_t timeout_ms) const;
  int WaitForever() const;
  void Wait(int64_t timeout_ms = 0LL) const;
  int WaitForTime(const timespec* abstime) const;

  virtual void Notify() const;
  virtual void NotifyAll() const;

 private:
  class Impl;
  Impl* impl_;

  DISALLOW_COPY_AND_ASSIGN(Monitor);
};

class Synchronized {
 public:
  explicit Synchronized(const Monitor* monitor) : g_(monitor->GetMutex()) {}
  explicit Synchronized(const Monitor& monitor) : g_(monitor.GetMutex()) {}
 private:
  Guard g_;
};

} // namespace threads
#endif // THREADS_MONITOR_H_
