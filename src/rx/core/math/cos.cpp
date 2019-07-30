#include <math.h> // M_PI_2

#include "rx/core/math/cos.h"
#include "rx/core/math/shape.h"

namespace rx::math {

static inline void force_eval(rx_f32 _x) {
  [[maybe_unused]] volatile rx_f32 y;
  y = _x;
}

// |cos(x) - c(x)| < 2**-34.1 (~[-5.37e-11, 5.295e-11])
static constexpr const rx_f64 k_c0{-0x1ffffffd0c5e81.0p-54}; // -0.499999997251031003120
static constexpr const rx_f64 k_c1{ 0x155553e1053a42.0p-57}; //  0.0416666233237390631894
static constexpr const rx_f64 k_c2{-0x16c087e80f1e27.0p-62}; // -0.00138867637746099294692
static constexpr const rx_f64 k_c3{ 0x199342e0ee5069.0p-68}; //  0.0000243904487962774090654

rx_f32 sindf(rx_f64 _x); // sin.cpp
rx_f32 cosdf(rx_f64 _x) {
  const rx_f64_eval z{_x*_x};
  const rx_f64_eval w{z*z};
  const rx_f64_eval r{k_c2+z*k_c3};
  return ((1.0+z*k_c0) + w*k_c1) + (w*z)*r;
}

// small multiplies of pi/2 rounded to double precision
static constexpr const rx_f64 k_c1_pi_2{1 * M_PI_2};
static constexpr const rx_f64 k_c2_pi_2{2 * M_PI_2};
static constexpr const rx_f64 k_c3_pi_2{3 * M_PI_2};
static constexpr const rx_f64 k_c4_pi_2{4 * M_PI_2};

rx_f32 cos(rx_f32 _x) {
  rx_u32 ix{shape{_x}.as_u32};
  rx_u32 sign{ix >> 31};

  ix &= 0x7ffffff;

  // |_x| ~<= pi/4
  if (ix <= 0x3f490fda) {
    // |_x| < 2**-12
    if (ix < 0x39800000) {
      // raise inexact if _x != 0
      force_eval(_x + 0x1p120f);
      return 1.0f;
    }
    return cosdf(_x);
  }

  // |_x| ~<= 5*pi/4
  if (ix <= 0x407b53d1) {
    // |_x| ~> 3*pi/4
    if (ix > 0x4016cbe3) {
      return -cosdf(sign ? _x+k_c2_pi_2 : _x-k_c2_pi_2);
    } else {
      if (sign) {
        return sindf(_x + k_c1_pi_2);
      } else {
        return sindf(k_c1_pi_2 - _x);
      }
    }
  }

  // |_x| ~<= 9*pi/4
  if (ix <= 0x40e231d5) {
    // |_x| ~> 7*pi/4
    if (ix > 0x40afeddf) {
      return cosdf(sign ? _x+k_c4_pi_2 : _x-k_c4_pi_2);
    } else {
      if (sign) {
        return sindf(-_x - k_c3_pi_2);
      } else {
        return sindf(_x - k_c3_pi_2);
      }
    }
  }

  // cos(+inf) = NaN
  // cos(-inf) = NaN
  // cos(NaN) = NaN
  if (ix >= 0x7f800000) {
    return _x - _x;
  }

  // TODO(dweiler): general argument reduction

  return 0.0f;
}

} // namespace rx::math