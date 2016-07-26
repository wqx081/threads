#ifndef THREADS_MUTEX_H_
#define THREADS_MUTEX_H_
#include "base/macros.h"

#include <memory>
#include <stdint.h>

namespace threads {

class Mutex {
 public:
  typedef void (*Initializer)(void*);

  static void DefaultInitializer(void*);
  static void AdaptiveInitializer(void*);
  static void RecursiveInitializer(void*);

  Mutex(Initializer init=DefaultInitializer);
  virtual ~Mutex() {}
  virtual void Lock() const;
  virtual bool TryLock() const;
  virtual bool TimedLock(int64_t milliseconds) const;
  virtual void Unlock() const;

  void* GetUnderlyingImpl() const;

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

class ReadWriteMutex {
 public:
  ReadWriteMutex();
  virtual ~ReadWriteMutex() {}

  virtual void AcquireRead() const;
  virtual void AcquireWrite() const;

  virtual bool AttemptRead() const;
  virtual bool AttemptWrite() const;

  virtual void Release() const;

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

class NoStarveReadWriteMutex : public ReadWriteMutex {
 public:
  NoStarveReadWriteMutex();

  virtual void AcquireRead() const;
  virtual void AcquireWrite() const;

 private:
  Mutex mutex_;
  mutable volatile bool writer_waiting_;  
};

class Guard {
 public:
  Guard(const Mutex& mutex, int64_t timeout=0) : mutex_(&mutex) {
    if (timeout == 0) {
      mutex.Lock();
    } else if (timeout < 0) {
      if (!mutex.TryLock()) {
        mutex_ = nullptr;
      }
    } else {
      if (!mutex.TimedLock(timeout)) {
        mutex_ = nullptr;
      }
    }
  }
  ~Guard() {
    if (mutex_) {
      mutex_->Unlock();
    }
  }

  operator bool() const {
    return mutex_ != nullptr;
  }

 private:
  const Mutex* mutex_;

  DISALLOW_COPY_AND_ASSIGN(Guard);
};

enum RWGuardType { RW_READ = 0, RW_WRITE = 1 };

class RWGuard {
 public:
  RWGuard(const ReadWriteMutex& mutex,
          bool write = false) : rw_mutex_(mutex) {
    if (write) {
      rw_mutex_.AcquireWrite();
    } else {
      rw_mutex_.AcquireRead();
    }
  }

  RWGuard(const ReadWriteMutex& mutex, RWGuardType type)
      : rw_mutex_(mutex) {
    if (type == RW_WRITE) {
      rw_mutex_.AcquireWrite();
    } else {
      rw_mutex_.AcquireRead();
    } 
  }

  ~RWGuard() {
    rw_mutex_.Release();
  }

 private:
  const ReadWriteMutex& rw_mutex_;

  DISALLOW_COPY_AND_ASSIGN(RWGuard);
};

} // namespace threads
#endif // THREADS_MUTEX_H_
