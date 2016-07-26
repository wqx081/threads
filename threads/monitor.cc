#include "threads/monitor.h"
#include "threads/exception.h"
#include "base/time_util.h"

#include <glog/logging.h>
#include <errno.h>
#include <pthread.h>

namespace threads {

class Monitor::Impl {
 public:
  Impl() : owned_mutex_(new Mutex()),
           mutex_(nullptr),
           cond_initialized_(false) {
    Init(owned_mutex_.get());
  }

  Impl(Mutex* mutex) : mutex_(nullptr), cond_initialized_(false) {
    Init(mutex);
  }

  Impl(Monitor* monitor) : mutex_(nullptr), cond_initialized_(false) {
    Init(&(monitor->GetMutex()));
  }

  ~Impl() { Cleanup(); }

  Mutex& GetMutex() { return *mutex_; }
  void Lock() { GetMutex().Lock(); }
  void Unlock() { GetMutex().Unlock(); }

  void Wait(int64_t timeout_ms) const {
    int ret = WaitForTimeRelative(timeout_ms);
    if (ret == ETIMEDOUT) {
      throw TimedOutException();
    } else if (ret != 0) {
      throw std::runtime_error("pthread_cond_wait() failed");
    }
  }
  int WaitForTimeRelative(int64_t timeout_ms) const {
    if (timeout_ms == 0LL) {
      return WaitForever();
    }
    struct timespec abstime;
    base::TimeUtil::ToTimespec(abstime, base::TimeUtil::CurrentTime() + timeout_ms);
    return WaitForTime(&abstime);
  }
  int WaitForTime(const timespec* abstime) const {
    assert(mutex_);
   
    pthread_mutex_t* mutex_impl = reinterpret_cast<pthread_mutex_t *>(mutex_->GetUnderlyingImpl());
    assert(mutex_impl);
    
    return pthread_cond_timedwait(&pthread_cond_,
                                  mutex_impl,
                                  abstime);
  }
  int WaitForever() const {
    assert(mutex_);

    pthread_mutex_t* mutex_impl = reinterpret_cast<pthread_mutex_t *>(mutex_->GetUnderlyingImpl());
    assert(mutex_impl);
    return pthread_cond_wait(&pthread_cond_, mutex_impl);
  }
  
  void Notify() {
    assert(mutex_ ); //mutex_->IsLocked());
    int ret = pthread_cond_signal(&pthread_cond_);
    DCHECK(ret == 0);
  }
  void NotifyAll() {
    assert(mutex_); // && mutex_->IsLocked());
 
    int ret = ::pthread_cond_broadcast(&pthread_cond_);
    DCHECK(ret == 0);
  }

 private:
  std::unique_ptr<Mutex> owned_mutex_;
  Mutex* mutex_;

  mutable pthread_cond_t pthread_cond_;
  mutable bool cond_initialized_;

  void Init(Mutex* mutex) {
    mutex_ = mutex;
    if (pthread_cond_init(&pthread_cond_, nullptr) == 0) {
      cond_initialized_ = true;
    }

    if (!cond_initialized_) {
      Cleanup();
      throw SystemResourceException();
    }
  }
  void Cleanup() {
    if (cond_initialized_) {
      cond_initialized_ = false;
      int ret = pthread_cond_destroy(&pthread_cond_);
      DCHECK(ret == 0);
    }
  }
};

Monitor::Monitor(): impl_(new Monitor::Impl()) {}
Monitor::Monitor(Mutex* mutex) : impl_(new Monitor::Impl(mutex)) {}
Monitor::Monitor(Monitor* monitor) : impl_(new Monitor::Impl(monitor)) {}

Monitor::~Monitor() { delete impl_; }

Mutex& Monitor::GetMutex() const {
  return impl_->GetMutex();
}

void Monitor::Lock() const {
  impl_->Lock();
}

void Monitor::Unlock() const {
  impl_->Unlock();
}

void Monitor::Wait(int64_t timeout) const {
  impl_->Wait(timeout);
}

int Monitor::WaitForTime(const timespec* abstime) const {
  return impl_->WaitForTime(abstime);
}

int Monitor::WaitForTimeRelative(int64_t timeout_ms) const {
  return impl_->WaitForTimeRelative(timeout_ms);
}

int Monitor::WaitForever() const {
  return impl_->WaitForever();
}

void Monitor::Notify() const {
  impl_->Notify();
}

void Monitor::NotifyAll() const {
  impl_->NotifyAll();
}

} // namespace threads
