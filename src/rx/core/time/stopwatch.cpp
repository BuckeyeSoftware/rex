#include "rx/core/time/stopwatch.h"
#include "rx/core/time/qpc.h"

namespace rx::time {

void stopwatch::reset() {
  m_range[0] = 0;
  m_range[1] = 0;
}

void stopwatch::restart() {
  reset();
  start();
}

void stopwatch::start() {
  m_range[0] = qpc_ticks();
}

void stopwatch::stop() {
  m_range[1] = qpc_ticks();
}

span stopwatch::elapsed() const {
  // NOTE(dweiler): QPC is a monotonic clock source. This means the invariant
  // m_range[1] >= m_range[0] is always true. This means that the subtraction
  // cannot underflow.
  return {m_range[1] - m_range[0], qpc_frequency()};
}

} // namespace rx::time
