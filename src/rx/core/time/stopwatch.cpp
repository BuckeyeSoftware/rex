#include "rx/core/time/stopwatch.h"
#include "rx/core/time/qpc.h"

namespace rx::time {

void stopwatch::start() {
  m_start_ticks = qpc_ticks();
}

void stopwatch::stop() {
  m_stop_ticks = qpc_ticks();
}

span stopwatch::elapsed() const {
  return {m_stop_ticks - m_start_ticks, qpc_frequency()};
}

} // namespace rx::time
