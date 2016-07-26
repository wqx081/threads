#ifndef THREADS_MUTEX_H_
#define THREADS_MUTEX_H_

#include "base/macros.h"

#include <cstdint>
#include <memory>

namespace threads {

class MutexImpl;
class RWMutexImpl;

class Mutex {
 public:
  explicit Mutex(int type=kDefaultInitializer);

  virtual ~Mutex() {}

  virtual void Lock() const;
  virtual bool TryLock() const;
  virtual bool TimedLock(int64_t milliseconds) const;
  virtual void Unlock() const;
  virtual bool IsLocked() const;
  void* GetUnderlyingImpl() const;

  static int kDefaultInitializer;
  static int kRecusiveInitializer;
 private:
  std::shared_ptr<MutexImpl> impl_;
};

class ReadWriteMutex {
 public:
  ReadWriteMutex();
  virtual ~ReadWriteMutex() {}
  
  virtual void AcquireRead() const;
  virtual void AcquireWrite() const;

  virtual bool TimedRead(int64_t milliseconds) const;
  virtual bool TimedWrite(int64_t milliseconds) const;

  virtual bool AttemptRead() const;
  virtual bool AttemptWrite() const;

  virtual void Release() const;

 private:
  std::shared_ptr<RWMutexImpl> impl_;
};

class NoStarveReadWriteMutex : public ReadWriteMutex {
 public:
  NoStarveReadWriteMutex();
// ~NoStarveReadWriteMutex();  

  void AcquireRead() const override;
  void AcquireWrite() const override;

  bool TimedRead(int64_t milliseconds) const override;
  bool TimedWrite(int64_t milliseconds) const override;

 private:
  Mutex mutex_;
  mutable volatile bool writer_waiting_;
};

class Guard {
 public:
  explicit Guard(const Mutex& mutex, int64_t timeout=0)
      : mutex_(&mutex) {
    if (timeout == 0) {
      mutex.Lock();
    } else if (timeout < 0) {
      if (mutex.TryLock()) {
        mutex_ = nullptr;
      }
    } else {
      if (mutex.TimedLock(timeout)) {
        mutex_ = nullptr;
      }
    }
  }

  ~Guard() {
    Release();
  }

  bool Release() {
    if (!mutex_) {
      return false;
    }
    mutex_->Unlock();
    mutex_ = nullptr;
    return true;
  }

  typedef const Mutex* const Guard::* pBoolMember;
  inline operator pBoolMember() const {
    return mutex_ != nullptr ? &Guard::mutex_ : nullptr;
  }

 private:
  const Mutex* mutex_;
  DISALLOW_COPY_AND_ASSIGN(Guard);
};

enum RWGuardType {
  RW_READ = 0,
  RW_WRITE = 1,  
};

class RWGuard {
 public:
  explicit RWGuard(const ReadWriteMutex& mutex, bool write = false,
                   int64_t timeout=0)
      : rw_mutex_(mutex), locked_(true) {
    if (write) {
      if (timeout) {
        locked_ = rw_mutex_.TimedWrite(timeout);
      } else {
        rw_mutex_.AcquireWrite();
      }
    } else {
      if (timeout) {
        locked_ = rw_mutex_.TimedRead(timeout);
      } else {
        rw_mutex_.AcquireRead();
      }
    }  
  }

  RWGuard(const ReadWriteMutex& mutex, RWGuardType type, int64_t timeout=0)
        : rw_mutex_(mutex), locked_(true) {
    if (type == RW_WRITE) {
      if (timeout) {
        locked_ = rw_mutex_.TimedWrite(timeout);
      } else {
        rw_mutex_.AcquireWrite();
      }
    } else {
      if (timeout) {
        locked_ = rw_mutex_.TimedRead(timeout);
      } else {
        rw_mutex_.AcquireRead();
      } 
    }
  }

  ~RWGuard() {
    if (locked_) {
      rw_mutex_.Release();
    }
  }

  typedef const bool RWGuard::* pBoolMember;
  operator pBoolMember() const {
    return locked_ ? &RWGuard::locked_ : nullptr;
  }
  bool operator!() const {
    return !locked_;
  }
  bool Release() {
    if (!locked_) {
      return false;
    }
    rw_mutex_.Release();
    locked_ = false;
    return true;
  }

 private:
  const ReadWriteMutex& rw_mutex_;
  mutable bool locked_;

  DISALLOW_COPY_AND_ASSIGN(RWGuard);
};


} // namespace threads
#endif // THREADS_MUTEX_H_
