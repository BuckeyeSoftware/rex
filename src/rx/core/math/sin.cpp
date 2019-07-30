#include <math.h> // M_PI_2

#include "rx/core/math/sin.h"
#include "rx/core/math/shape.h"

namespace rx::math {

static inline void force_eval(rx_f32 _x) {
  [[maybe_unused]] volatile rx_f32 y;
  y = _x;
}

// |sin(x)/x - s(x)| < 2**-37.5 (~[-4.89e-12, 4.824e-12])
static constexpr const rx_f64 k_s1{-0x15555554cbac77.0p-55}; // -0.166666666416265235595
static constexpr const rx_f64 k_s2{0x111110896efbb2.0p-59};  //  0.0083333293858894631756
static constexpr const rx_f64 k_s3{-0x1a00f9e2cae774.0p-65}; // -0.000198393348360966317347
static constexpr const rx_f64 k_s4{0x16cd878c3b46a7.0p-71};  //  0.0000027183114939898219064

rx_f32 cosdf(rx_f64 _x); // cos.cpp
rx_f32 sindf(rx_f64 _x) {
  const rx_f64_eval z{_x*_x};
  const rx_f64_eval w{z*z};
  const rx_f64_eval r{k_s3 + z*k_s4};
  const rx_f64_eval s{z*_x};
  return (_x + s*(k_s1 + z*k_s2)) + s*w*r;
}

// small multiplies of pi/2 rounded to double precision
static constexpr rx_f64 k_s1_pi_2{1*M_PI_2};
static constexpr rx_f64 k_s2_pi_2{2*M_PI_2};
static constexpr rx_f64 k_s3_pi_2{3*M_PI_2};
static constexpr rx_f64 k_s4_pi_2{4*M_PI_2};

rx_f32 sin(rx_f32 _x) {
  rx_u32 ix{shape{_x}.as_u32};
  rx_u32 sign{ix >> 31};

  ix &= 0x7ffffff;

  // |_x| ~<= pi/4
  if (ix <= 0x3f490fda) {
    // |_x| < 2**-12
    if (ix < 0x39800000) {
      // raise inexact if x != 0 and underflow if subnormal
      force_eval(ix < 0x00800000 ? _x/0x1p120f : _x+0x1p120f);
      return _x;
    }
    return sindf(_x);
  }

  // |_x| ~<= 5*pi/4
  if (ix <= 0x407b53d1) {
    // |_x| ~<= 3*pi/4
    if (ix <= 0x4016cbe3) {
      if (sign) {
        return -cosdf(_x + k_s1_pi_2);
      } else {
        return cosdf(_x - k_s1_pi_2);
      }
    }
    return sindf(sign ? -(_x + k_s2_pi_2) : -(_x - k_s2_pi_2));
  }

  // |_x| ~<= 9*pi/4
  if (ix <= 0x40e231d5) {
    // |_x| ~<= 7*pi/4
    if (ix <= 0x40afeddf) {
      if (sign) {
        return cosdf(_x + k_s3_pi_2);
      } else {
        return -cosdf(_x - k_s3_pi_2);
      }
    }
    return sindf(sign ? _x + k_s4_pi_2 : _x - k_s4_pi_2);
  }

  // sin(+inf) = NaN
  // sin(-inf) = NaN
  // sin(NaN) = NaN
  if (ix >= 0x7f800000) {
    return _x - _x;
  }

  // TODO(dweiler): general argument reduction

  return 0.0f;
}

} // namespace rx::math