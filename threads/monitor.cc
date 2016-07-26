#include "threads/monitor.h"
#include "threads/exception.h"
#include "base/time_util.h"

#include <glog/logging.h>
#include <errno.h>
#include <pthread.h>

namespace threads {

class Monitor::Impl {
 private:
  std::unique_ptr<Mutex> owned_mutex_;
  Mutex* mutex_;

  mutable pthread_cond_t pthread_cond_;
  mutable bool cond_initialized_;

  void Init(Mutex* mutex) {
    
  }
};

} // namespace threads
