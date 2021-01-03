#ifndef RX_MATH_TRIG_H
#define RX_MATH_TRIG_H
#include "rx/core/math/constants.h" // PI<T>

namespace Rx::Math {

template<typename T>
constexpr T deg_to_rad(T deg) {
  return deg * PI<T> / T{180.0};
}

template<typename T>
constexpr T rad_to_deg(T rad) {
  return rad * T{180.0} / PI<T>;
}

} // namespace Rx::Math

#endif // RX_MATH_TRIG_H
