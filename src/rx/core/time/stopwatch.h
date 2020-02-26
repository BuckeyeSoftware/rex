#ifndef RX_CORE_TIME_STOPWATCH_H
#define RX_CORE_TIME_STOPWATCH_H
#include "rx/core/time/span.h"

namespace rx::time {

struct stopwatch {
  constexpr stopwatch();

  void reset();
  void restart();
  void start();
  void stop();

  bool is_running() const;

  span elapsed() const;

private:
  rx_u64 m_start_ticks;
  rx_u64 m_stop_ticks;
};

constexpr stopwatch::stopwatch()
  : m_start_ticks{0}
  , m_stop_ticks{0}
{
}

inline void stopwatch::reset() {
  m_start_ticks = 0;
  m_stop_ticks = 0;
}

inline void stopwatch::restart() {
  reset();
  start();
}

inline bool stopwatch::is_running() const {
  return m_start_ticks != 0;
}

} // namespace rx::time

#endif // RX_CORE_TIME_STOPWATCH_H
