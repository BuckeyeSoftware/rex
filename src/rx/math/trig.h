#ifndef RX_MATH_TRIG_H
#define RX_MATH_TRIG_H

#include "rx/core/types.h" // f32, f64
#include "rx/math/constants.h" // k_pi

namespace rx::math {

template<typename T>
inline constexpr T deg_to_rad(T deg) {
  return deg * k_pi<T> / T{180.0};
}

template<typename T>
inline constexpr T rad_to_deg(T rad) {
  return rad * T{180.0} / k_pi<T>;
}

rx_f32 sin(rx_f32 _x);
rx_f32 cos(rx_f32 _x);
rx_f32 tan(rx_f32 _x);

} // namespace rx::math

#endif // RX_MATH_TRIG_H
