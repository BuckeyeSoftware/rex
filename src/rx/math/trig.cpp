#include "rx/math/trig.h"

// we define libm functions directly here to avoid including math.h, we only use
// a handful of the functions anyways
extern "C" {
float sinf(float);
float cosf(float);
float tanf(float);
}

namespace rx::math {

rx_f32 sin(rx_f32 _x) {
  return ::sinf(_x);
}

rx_f32 cos(rx_f32 _x) {
  return ::cosf(_x);
}

rx_f32 tan(rx_f32 _x) {
  return ::tanf(_x);
}

} // namespace rx::math