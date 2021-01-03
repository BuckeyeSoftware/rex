#ifndef RX_CORE_ALGORITHM_SATURATE_H
#define RX_CORE_ALGORITHM_SATURATE_H
#include "rx/core/hints/force_inline.h"

namespace Rx::Algorithm {

// Saturate value |_value| to [0, 1].
template<typename T>
RX_HINT_FORCE_INLINE constexpr T saturate(T _value) {
  return _value < T(0) ? T(0) : (_value > T(1) ? T(1) : _value);
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_SATURATE_H
