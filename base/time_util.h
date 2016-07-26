#ifndef BASE_TIME_UTIL_H_
#define BASE_TIME_UTIL_H_
#include "base/macros.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <chrono>

namespace base {

class TimeUtil {
 public:

  static const int64_t NS_PER_S = 1000000000LL;
  static const int64_t US_PER_S = 1000000LL;
  static const int64_t MS_PER_S = 1000LL;

  static const int64_t NS_PER_MS = NS_PER_S / MS_PER_S;
  static const int64_t NS_PER_US = NS_PER_S / US_PER_S;
  static const int64_t US_PER_MS = US_PER_S / MS_PER_S;

  static void ToTimespec(struct timespec& result, int64_t value) {
    result.tv_sec = value / MS_PER_S; // ms to s
    result.tv_nsec = (value % MS_PER_S) * NS_PER_MS; // ms to ns
  }

  static void ToTimeval(struct timeval& result, int64_t value) {
    result.tv_sec = value / MS_PER_S; // ms to s
    result.tv_usec = (value % MS_PER_S) * US_PER_MS; // ms to us
  }

  static void ToTicks(int64_t& result, int64_t secs, int64_t old_ticks,
                      int64_t old_ticks_per_sec, int64_t new_ticks_per_sec) {
    result = secs * new_ticks_per_sec;
    result += old_ticks * new_ticks_per_sec / old_ticks_per_sec;

    int64_t old_per_new = old_ticks_per_sec / new_ticks_per_sec;
    if (old_per_new && ((old_ticks % old_per_new) >= (old_per_new / 2))) {
      ++result;
    }
  }

  static void ToTicks(int64_t& result,
                      const struct timespec& value,
                      int64_t ticks_per_sec) {
    return ToTicks(result, value.tv_sec, value.tv_nsec, NS_PER_S, ticks_per_sec);
  }

  static void ToTicks(int64_t& result,
                      const struct timeval& value,
                      int64_t ticks_per_sec) {
    return ToTicks(result, value.tv_sec, value.tv_usec, US_PER_S, ticks_per_sec);
  }

  static void ToMilliseconds(int64_t& result,
                             const struct timespec& value) {
    return ToTicks(result, value, MS_PER_S);
  }

  static void ToMilliseconds(int64_t& result,
                             const struct timeval& value) {
    return ToTicks(result, value, MS_PER_S);
  }

  static void ToUsec(int64_t& result, const struct timespec& value) {
    return ToTicks(result, value, US_PER_S);
  }

  static void ToUsec(int64_t& result, const struct timeval& value) {
    return ToTicks(result, value, US_PER_S);
  }

  static int64_t CurrentTimeTicks(int64_t ticks_per_sec);
  static int64_t CurrentTime() { return CurrentTimeTicks(MS_PER_S); }
  static int64_t CurrentTimeUsec() { return CurrentTimeTicks(US_PER_S); }

  static int64_t MonotonicTimeTicks(int64_t ticks_per_sec);
  static int64_t MonotonicTime() { return MonotonicTimeTicks(MS_PER_S); }
  static int64_t MonotonicTimeUsec() {
    return MonotonicTimeTicks(US_PER_S);
  }

};

} // namespace base 
#endif // BASE_TIME_UTIL_H_
