#ifndef RX_CORE_ALGORITHM_CLAMP_H
#define RX_CORE_ALGORITHM_CLAMP_H
#include "rx/core/hints/force_inline.h"

namespace Rx::Algorithm {

// Clamp value |_value| to the given range [|_min| |_max|].
template<typename T>
RX_HINT_FORCE_INLINE constexpr T clamp(T _value, T _min, T _max) {
  return _value < _min ? _min : (_value > _max ? _max : _value);
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_CLAMP_H
