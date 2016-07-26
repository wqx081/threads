#include "threads/mutex.h"
#include "threads/mutex_impl.h"
#include "base/time_util.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

namespace threads {

int Mutex::kDefaultInitializer = PTHREAD_MUTEX_NORMAL;
int Mutex::kRecusiveInitializer = PTHREAD_MUTEX_RECURSIVE;

Mutex::Mutex(int type)
    : impl_(std::make_shared<MutexImpl>(type)) {}

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
  return impl_->TryLockFor(std::chrono::milliseconds {ms});
}

void Mutex::Unlock() const {
  impl_->Unlock();
}

bool Mutex::IsLocked() const {
  return impl_->IsLocked();
}

///////////////////////////////////////////////////
ReadWriteMutex::ReadWriteMutex()
  : impl_(std::make_shared<RWMutexImpl>()) {}

void ReadWriteMutex::AcquireRead() const {
  impl_->LockShared();
}

void ReadWriteMutex::AcquireWrite() const {
  impl_->Lock();
}

bool ReadWriteMutex::TimedRead(int64_t milliseconds) const {
  return impl_->TryLockSharedFor(std::chrono::milliseconds{milliseconds});
}

bool ReadWriteMutex::TimedWrite(int64_t milliseconds) const {
  return impl_->TryLockFor(std::chrono::milliseconds{milliseconds});
}

bool ReadWriteMutex::AttemptRead() const {
  return impl_->TryLockShared();
}

bool ReadWriteMutex::AttemptWrite() const {
  return impl_->TryLock();
}

void ReadWriteMutex::Release() const {
  return impl_->Unlock();
}

///////////////////////////////////////////////////
NoStarveReadWriteMutex::NoStarveReadWriteMutex()
  : writer_waiting_(false) {}

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

bool NoStarveReadWriteMutex::TimedRead(int64_t milliseconds) const {
  if (writer_waiting_) {
    if (!mutex_.TimedLock(milliseconds)) {
      return false;
    }
    mutex_.Unlock();
  }
  return ReadWriteMutex::TimedRead(milliseconds);
}

bool NoStarveReadWriteMutex::TimedWrite(int64_t milliseconds) const {
  if (AttemptWrite()) {
    return true;
  }

  if (!mutex_.TimedLock(milliseconds)) {
    return false;
  }

  writer_waiting_ = true;
  bool ret = ReadWriteMutex::TimedWrite(milliseconds);
  writer_waiting_ = false;
  mutex_.Unlock();
  return ret;
}

} // namespace threads
