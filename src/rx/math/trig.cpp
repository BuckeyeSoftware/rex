#include <math.h> // {cos,sin,sqrt}f

#include <rx/math/trig.h>

namespace rx::math {

rx_f32 sin(rx_f32 _x) {
  return ::sinf(_x);
}

rx_f32 cos(rx_f32 _x) {
  return ::cosf(_x);
}

rx_f32 sqrt(rx_f32 _x) {
  return ::sqrtf(_x);
}

rx_f32 abs(rx_f32 _x) {
  return ::fabsf(_x);
}

rx_f64 sin(rx_f64 _x) {
  return ::sin(_x);
}

rx_f64 cos(rx_f64 _x) {
  return ::cos(_x);
}

rx_f64 sqrt(rx_f64 _x) {
  return ::sqrt(_x);
}

rx_f64 abs(rx_f64 _x) {
  return ::fabs(_x);
}

} // namespace rx::math