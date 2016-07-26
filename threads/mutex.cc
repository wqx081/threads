#include "threads/mutex.h"
#include "base/time_util.h"
#include <pthread.h>
#include <signal.h>
#include <glog/logging.h>

namespace threads {

class Mutex::Impl {
 public:
  Impl(Initializer Init) : initialized_(false) {
    Init(&pthread_mutex_);
    initialized_ = true;
  }
  ~Impl() {
    if (initialized_) {
      initialized_ = false;
      int ret = ::pthread_mutex_destroy(&pthread_mutex_);
      DCHECK(ret == 0);
    }
  }
  void Lock() const {
    pthread_mutex_lock(&pthread_mutex_);
  }
  bool TryLock() const {
    return (0 == pthread_mutex_trylock(&pthread_mutex_));
  }
  bool TimedLock(int milliseconds) const {
    timespec ts;
    base::TimeUtil::ToTimespec(ts, 
		    milliseconds + base::TimeUtil::CurrentTime());
    int ret = ::pthread_mutex_timedlock(&pthread_mutex_, &ts);
    if (ret == 0) {
      return true;
    }
    return false;
  }
  void Unlock() const {
    ::pthread_mutex_unlock(&pthread_mutex_);
  }
  void* GetUnderlyingImpl() const {
    return static_cast<void*>(&pthread_mutex_);
  }

 private:
  mutable pthread_mutex_t pthread_mutex_;
  mutable bool initialized_;
};

Mutex::Mutex(Initializer Init) : impl_(new Mutex::Impl(Init)) {}

void* Mutex::GetUnderlyingImpl() const {
  return impl_->GetUnderlyingImpl();
}

void Mutex::Lock() const {
  impl_->Lock();
}

bool Mutex::TryLock() const {
  return impl_->TryLock();
}

bool Mutex::TimedLock(int64_t ms) const {
  return impl_->TimedLock(ms);
}

void Mutex::Unlock() const {
  impl_->Unlock();
}

void Mutex::DefaultInitializer(void* arg) {
  pthread_mutex_t* mutex = static_cast<pthread_mutex_t *>(arg);
  int ret = ::pthread_mutex_init(mutex, nullptr);
  DCHECK(ret == 0);
}

static void InitWithKind(pthread_mutex_t* mutex, int kind) {
  pthread_mutexattr_t mutex_attr;
  int ret = ::pthread_mutexattr_init(&mutex_attr);
  DCHECK(ret == 0);

  ret = ::pthread_mutexattr_settype(&mutex_attr, kind);
  DCHECK(ret == 0);

  ret = ::pthread_mutex_init(mutex, &mutex_attr);
  DCHECK(ret == 0);

  ret = ::pthread_mutexattr_destroy(&mutex_attr);
  DCHECK(ret == 0);
}

void Mutex::AdaptiveInitializer(void* arg) {
  InitWithKind(static_cast<pthread_mutex_t *>(arg),
		PTHREAD_MUTEX_ADAPTIVE_NP);  
}

void Mutex::RecursiveInitializer(void* arg) {
  InitWithKind(static_cast<pthread_mutex_t *>(arg),
               PTHREAD_MUTEX_RECURSIVE_NP);
}

///////////////////
class ReadWriteMutex::Impl {
 public:
  Impl() : initialized_(false) {
    int ret = ::pthread_rwlock_init(&rw_lock_, nullptr);
    DCHECK(ret == 0);
    initialized_ = true;
  }
  ~Impl() {
    if (initialized_) {
      initialized_ = false;
      int ret = ::pthread_rwlock_destroy(&rw_lock_);
      DCHECK(ret == 0);
    }
  }
  void AcquireRead() const {
    ::pthread_rwlock_rdlock(&rw_lock_);
  }
  void AcquireWrite() const {
    ::pthread_rwlock_wrlock(&rw_lock_);
  }
  bool AttemptRead() const {
    return !::pthread_rwlock_tryrdlock(&rw_lock_);
  }
  bool AttemptWrite() const {
    return !::pthread_rwlock_trywrlock(&rw_lock_);
  }

  void Release() const {
    ::pthread_rwlock_unlock(&rw_lock_);
  }

 private:
  mutable pthread_rwlock_t rw_lock_;
  mutable bool initialized_;
};

ReadWriteMutex::ReadWriteMutex() : impl_(new ReadWriteMutex::Impl()) {}

void ReadWriteMutex::AcquireRead() const {
  impl_->AcquireRead();
}

void ReadWriteMutex::AcquireWrite() const {
  impl_->AcquireWrite();
}

bool ReadWriteMutex::AttemptRead() const {
  return impl_->AttemptRead();
}

bool ReadWriteMutex::AttemptWrite() const {
  return impl_->AttemptWrite();
}

void ReadWriteMutex::Release() const {
  impl_->Release();
}

NoStarveReadWriteMutex::NoStarveReadWriteMutex() : writer_waiting_(false) {
}

void NoStarveReadWriteMutex::AcquireRead() const {
  if (writer_waiting_) {
    mutex_.Lock();
    mutex_.Unlock();
  }

  ReadWriteMutex::AcquireRead();
}

void NoStarveReadWriteMutex::AcquireWrite() const {
  if (AttemptWrite()) {
    return;
  }

  mutex_.Lock();
  writer_waiting_ = true;
  ReadWriteMutex::AcquireWrite();
  writer_waiting_ = false;
  mutex_.Unlock();
}

} // namespace threads
