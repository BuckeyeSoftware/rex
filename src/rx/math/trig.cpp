#include <rx/math/trig.h>

// we define libm functions directly here to avoid including math.h, we only use
// a handful of the functions anyways
extern "C" {
float sinf(float);
float cosf(float);
float fabsf(float);
float sqrtf(float);
float fmodf(float, float);

double sin(double);
double cos(double);
double sqrt(double);
double fabs(double);
double fmod(double, double);
}

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

rx_f32 mod(rx_f32 _x, rx_f32 _y) {
  return ::fmodf(_x, _y);
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

rx_f64 mod(rx_f64 _x, rx_f64 _y) {
  return ::fmod(_x, _y);
}

} // namespace rx::math