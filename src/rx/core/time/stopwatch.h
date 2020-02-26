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
  rx_u64 m_range[2];
};

constexpr stopwatch::stopwatch()
  : m_range{0, 0}
{
}

inline bool stopwatch::is_running() const {
  return m_range[0] != 0;
}

} // namespace rx::time

#endif // RX_CORE_TIME_STOPWATCH_H
