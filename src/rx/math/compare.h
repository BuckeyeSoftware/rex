#ifndef RX_MATH_COMPARE_H
#define RX_MATH_COMPARE_H
#include "rx/core/math/abs.h" // abs
#include "rx/core/algorithm/max.h" // max

namespace rx::math {

inline bool epsilon_compare(rx_f32 _x, rx_f32 _y) {
  return abs(_x - _y) <= k_epsilon<rx_f32> * algorithm::max(1.0f, _x, _y);
}

} // namespace rx::math

#endif // RX_MATH_COMPARE_H