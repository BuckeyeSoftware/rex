#include "rx/core/time/qpc.h"
#include "rx/core/config.h"

#if defined(RX_PLATFORM_POSIX)
#include <time.h>
#elif defined(RX_PLATFORM_WINDOWS)
#else
#error "missing qpc implementation"
#endif

namespace rx::time {

rx_u64 qpc_ticks() {
#if defined(RX_PLATFORM_POSIX)
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  rx_u64 ticks{0};
  ticks += now.tv_sec;
  ticks *= 1000000000;
  ticks += now.tv_nsec;

  return ticks;
#elif defined(RX_PLATFORM_WINDOWS)
  // TODO(dweiler): implement for Windows.
#endif
  return 0;
}

rx_u64 qpc_frequency() {
#if defined(RX_PLATFORM_POSIX)
  return 1000000000;
#elif defined(RX_PLATFORM_WINDOWS)
  // TODO(dweiler): implement for Windows.
#endif
}

} // namespace rx::time
