#ifndef THREADS_MUTEX_IMPL_H_
#define THREADS_MUTEX_IMPL_H_
#include "base/macros.h"
#include "base/time_util.h"

#include <pthread.h>
#include <glog/logging.h>

namespace threads {

class MutexImpl {
 public:
  explicit MutexImpl(int type) {
    pthread_mutexattr_t mutex_attr;
    CHECK(0 == pthread_mutexattr_init(&mutex_attr));
    CHECK(0 == pthread_mutexattr_settype(&mutex_attr, type));
    // Init pthread Mutex
    CHECK(0 == pthread_mutex_init(&pthread_mutex_, &mutex_attr));
    // Destroy mutex_attr
    CHECK(0 == pthread_mutexattr_destroy(&mutex_attr));
  }

  ~MutexImpl() {
    CHECK(0 == pthread_mutex_destroy(&pthread_mutex_));
  }

  void Lock() {
    int ret = pthread_mutex_lock(&pthread_mutex_);
    CHECK(ret != EDEADLK);
  }

  bool TryLock() {
    return (0 == ::pthread_mutex_trylock(&pthread_mutex_));
  }

  template<typename Rep, typename Period>
  bool TryLockFor(const std::chrono::duration<Rep, Period>& timeout_duration) {
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    struct timespec ts;
    base::TimeUtil::ToTimespec(ts, base::TimeUtil::CurrentTime() + duration_ms.count());
    return 0 == pthread_mutex_timedlock(&pthread_mutex_,&ts);
  }

  void Unlock() {
    int ret = pthread_mutex_unlock(&pthread_mutex_);
    CHECK(ret != EPERM);
  }

  bool IsLocked() {
    if (TryLock()) {
      Unlock();
      return false;
    }
    return true;
  }

  void* GetUnderlyingImpl() const {
    return (void*) &pthread_mutex_;
  }

 private:
  mutable pthread_mutex_t pthread_mutex_;
};

class RWMutexImpl {
 public:
  RWMutexImpl() {
    CHECK(0 == pthread_rwlock_init(&rw_lock_, nullptr));
  }

  ~RWMutexImpl() {
    CHECK(0 == pthread_rwlock_destroy(&rw_lock_));
  }

  void Lock() {
    int ret = pthread_rwlock_wrlock(&rw_lock_);
    CHECK(ret != EDEADLK);
  }

  bool TryLock() {
    return !pthread_rwlock_trywrlock(&rw_lock_);
  }
 
  template<typename Rep, typename Period>
  bool TryLockFor(const std::chrono::duration<Rep, Period>& timeout_duration) {
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    struct timespec ts;
    base::TimeUtil::ToTimespec(ts, base::TimeUtil::CurrentTime() + duration_ms.count());
    return 0 == pthread_rwlock_timedwrlock(&rw_lock_, &ts);
  }

  void Unlock() {
    pthread_rwlock_unlock(&rw_lock_);
  }

  void LockShared() {
    int ret = pthread_rwlock_rdlock(&rw_lock_);
    CHECK(ret != EDEADLK);
  }

  bool TryLockShared() {
    return !pthread_rwlock_tryrdlock(&rw_lock_);
  }
  
  template<typename Rep, typename Period>
  bool TryLockSharedFor(const std::chrono::duration<Rep, Period>& timeout_duration) {
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    struct timespec ts;
    base::TimeUtil::ToTimespec(ts, base::TimeUtil::CurrentTime() + duration_ms.count());
    return 0 == pthread_rwlock_timedrdlock(&rw_lock_, &ts);
  }

  void UnlockShared() {
    Unlock();
  }

 private:
  mutable pthread_rwlock_t rw_lock_;
};

} // namespace threads
#endif // THREADS_MUTEX_IMPL_H_
