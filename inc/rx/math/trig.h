#ifndef RX_MATH_TRIG_H
#define RX_MATH_TRIG_H

#include <rx/core/types.h> // f32, f64
#include <rx/math/constants.h> // k_pi

namespace rx::math {

template<typename T>
inline constexpr T deg_to_rad(T deg) {
  return deg * k_pi<T> / 180.0;
}

template<typename T>
inline constexpr T rad_to_deg(T rad) {
  return rad * 180.0 / k_pi<T>;
}

rx_f32 sin(rx_f32 x);
rx_f32 cos(rx_f32 x);

}

#endif
