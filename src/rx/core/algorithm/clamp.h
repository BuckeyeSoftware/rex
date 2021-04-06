#ifndef RX_CORE_ALGORITHM_CLAMP_H
#define RX_CORE_ALGORITHM_CLAMP_H
#include "rx/core/hints/force_inline.h"

/// \file clamp.h

/// Algorithms.
namespace Rx::Algorithm {

/// Clamp value \p _value to the given range [ \p _min , \p _max ].
/// \param _value The value to clamp
/// \param _min The minimum value
/// \param _max The maximum value
/// \return The clamped value
template<typename T>
RX_HINT_FORCE_INLINE constexpr T clamp(T _value, T _min, T _max) {
  return _value < _min ? _min : (_value > _max ? _max : _value);
}

} // namespace Rx::Algorithm

#endif // RX_CORE_ALGORITHM_CLAMP_H
