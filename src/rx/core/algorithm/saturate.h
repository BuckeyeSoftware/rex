#ifndef RX_CORE_ALGORITHM_SATURATE_H
#define RX_CORE_ALGORITHM_SATURATE_H
#include "rx/core/hints/force_inline.h"
#include "rx/core/concepts/floating_point.h"

/// \file saturate.h

namespace Rx::Algorithm {

/// Saturate floating point value to [0, 1] range.
/// \param _value The value to saturate.
/// \returns The saturated value.
RX_HINT_FORCE_INLINE constexpr auto saturate(Concepts::FloatingPoint auto _value) {
  using T = decltype(_value);
  return _value < T(0) ? T(0) : (_value > T(1) ? T(1) : _value);
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_SATURATE_H
