#include "base/time_util.h"
#include <glog/logging.h>

namespace base {

int64_t TimeUtil::CurrentTimeTicks(int64_t ticks_per_sec) {
  int64_t result;

  struct timespec now;
  int ret = clock_gettime(CLOCK_REALTIME, &now);
  DCHECK(ret == 0);
  ToTicks(result, now, ticks_per_sec);

  return result;
}

int64_t TimeUtil::MonotonicTimeTicks(int64_t ticks_per_sec) {
  static bool use_realtime;
  if (use_realtime) {
    return CurrentTimeTicks(ticks_per_sec);
  }

  struct timespec now;
  int ret = clock_gettime(CLOCK_MONOTONIC, &now);
  if (ret != 0) {
    assert(errno == EINVAL);
    use_realtime = true;
    return CurrentTimeTicks(ticks_per_sec);
  }

  int64_t result;
  ToTicks(result, now, ticks_per_sec);
  return result;
}

} // namespace base
