#ifndef RX_MATH_COMPARE_H
#define RX_MATH_COMPARE_H
#include "rx/core/math/abs.h" // abs
#include "rx/core/algorithm/max.h" // max

namespace Rx::Math {

inline bool epsilon_compare(Float32 _x, Float32 _y) {
  return abs(_x - _y) <= EPSILON<Float32> * Algorithm::max(1.0f, _x, _y);
}

} // namespace Rx::Math

#endif // RX_MATH_COMPARE_H
