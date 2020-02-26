#ifndef RX_CORE_TIME_SPAN_H
#define RX_CORE_TIME_SPAN_H
#include "rx/core/format.h"

namespace rx::time {

struct span {
  constexpr span(rx_u64 _start_ticks, rx_u64 _stop_ticks, rx_u64 _frequency);
  constexpr span(rx_u64 _ticks, rx_u64 _frequency);
  constexpr span(rx_s64 _ticks, rx_u64 _frequency);

  // The number of days, hours, minutes & milliseconds of this span.
  rx_s64 days() const;
  rx_s64 hours() const;
  rx_s64 minutes() const;
  rx_s64 seconds() const;
  rx_f64 milliseconds() const;

  rx_f64 total_days() const;
  rx_f64 total_hours() const;
  rx_f64 total_minutes() const;
  rx_f64 total_seconds() const;
  rx_f64 total_milliseconds() const;

private:
  constexpr span(bool _negate, rx_u64 _ticks, rx_u64 _frequency);

  // Store sign separately from |m_ticks| for representing ranges that go in
  // the opposite direction. This increases our effective time span range by
  // a factor of two.
  //
  // This also acts as a useful type-conversion since all calculations are
  // done on unsigned integers, the multiplication by a signed sign makes
  // everything signed, avoiding the need for explicit casts.
  rx_s64 m_sign;
  rx_u64 m_ticks;     // Number of ticks.
  rx_u64 m_frequency; // Number of ticks per second.
};

constexpr span::span(rx_u64 _start_ticks, rx_u64 _stop_ticks, rx_u64 _frequency)
  : span{_start_ticks < _stop_ticks ? _stop_ticks - _start_ticks : _start_ticks - _stop_ticks, _frequency}
{
}

constexpr span::span(rx_u64 _ticks, rx_u64 _frequency)
  : span{false, _ticks, _frequency}
{
}

constexpr span::span(rx_s64 _ticks, rx_u64 _frequency)
  : span{_ticks < 0, static_cast<rx_u64>(_ticks < 0 ? -_ticks : _ticks), _frequency}
{
}

constexpr span::span(bool _negate, rx_u64 _ticks, rx_u64 _frequency)
  : m_sign{_negate ? -1 : 1}
  , m_ticks{_ticks}
  , m_frequency{_frequency}
{
}

} // namespace rx::time

namespace rx {
  template<>
  struct format_type<time::span> {
    // Largest possible string is:
    // "%dd:%02dh:%02dm:%02ds:%.2fms"
    char scratch[format_size<rx_s64>::size + sizeof ":  :  :  :    ms"];
    const char* operator()(const time::span& _value);
  };
}

#endif // RX_CORE_TIME_SPAN_H
