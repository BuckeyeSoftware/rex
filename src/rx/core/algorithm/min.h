#ifndef RX_CORE_ALGORITHM_MIN_H
#define RX_CORE_ALGORITHM_MIN_H
#include "rx/core/utility/forward.h"
#include "rx/core/hints/force_inline.h"

/// \file min.h

namespace Rx::Algorithm {

/// Base case.
template<typename T>
RX_HINT_FORCE_INLINE constexpr T min(T _value) {
  return _value;
}

/// Get the minimum value of a list of values
template<typename T, typename... Ts>
RX_HINT_FORCE_INLINE constexpr T min(T _a, T _b, Ts&&... _args) {
  return min(_a < _b ? _a : _b, Utility::forward<Ts>(_args)...);
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_MIN_H
