#define _USE_MATH_DEFINES
#include <math.h> // M_PI_2

#include "rx/core/math/tan.h"
#include "rx/core/math/shape.h"
#include "rx/core/math/force_eval.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Math {
// |tan(x)/x - t(x)| < 2**-25.5 (~[-2e-08, 2e-08])
static constexpr const Float64 k_t0{0x15554d3418c99f.0p-54}; // 0.333331395030791399758
static constexpr const Float64 k_t1{0x1112fd38999f72.0p-55}; // 0.133392002712976742718
static constexpr const Float64 k_t2{0x1b54c91d865afe.0p-57}; // 0.0533812378445670393523
static constexpr const Float64 k_t3{0x191df3908c33ce.0p-58}; // 0.0245283181166547278873
static constexpr const Float64 k_t4{0x185dadfcecf44e.0p-61}; // 0.00297435743359967304927
static constexpr const Float64 k_t5{0x1362b9bf971bcd.0p-59}; // 0.00946564784943673166728

template<bool Odd>
static Float32 tandf(Float64 _x) {
  const Float64Eval z{_x * _x};
  const Float64Eval r{k_t4 + z * k_t5};
  const Float64Eval t{k_t2 + z * k_t3};
  const Float64Eval w{z * z};
  const Float64Eval s{z * _x};
  const Float64Eval u{k_t0 + z * k_t1};
  const Float64Eval l{(_x + s * u) + (s * w) * (t + w * r)};
  return static_cast<Float32>(Odd ? -1.0 / l : l);
}

// small multiples of pi/2 rounded to double precision
static constexpr const Float64 k_t1_pi_2{1 * M_PI_2};
static constexpr const Float64 k_t2_pi_2{2 * M_PI_2};
static constexpr const Float64 k_t3_pi_2{3 * M_PI_2};
static constexpr const Float64 k_t4_pi_2{4 * M_PI_2};

Sint32 rempio2(Float32 _x, Float64& y_); // sin.cpp

Float32 tan(Float32 _x) {
  Uint32 ix{Shape{_x}.as_u32};
  const Uint32 sign{ix >> 31};

  ix &= 0x7fffffff;

  // |_x| ~<= pi/4
  if (ix <= 0x3f490fda) {
    // |_x| < 2**-12
    if (ix < 0x39800000) {
      // raise inexact if x != 0 and underflow if subnormal
      force_eval_f32(ix < 0x00800000 ? _x/0x1p120f : _x+0x1p120f);
      return _x;
    }
    return tandf<false>(_x);
  }

  // |_x| ~<= 5*pi/4
  if (ix <= 0x407b53d1) {
    // |_x| ~<= 3*pi/4
    if (ix <= 0x4016cbe3) {
      return tandf<true>(sign ? _x+k_t1_pi_2 : _x-k_t1_pi_2);
    } else {
      return tandf<false>(sign ? _x+k_t2_pi_2 : _x-k_t2_pi_2);
    }
  }

  // |_x| ~<= 9*pi/4
  if (ix <= 0x40e231d5) {
    // |_x| ~<= 7*pi/4
    if (ix <= 0x40afeddf) {
      return tandf<true>(sign ? _x+k_t3_pi_2 : _x-k_t3_pi_2);
    } else {
      return tandf<false>(sign ? _x+k_t4_pi_2 : _x-k_t4_pi_2);
    }
  }

  // tan(+inf) = NaN
  // tan(-inf) = NaN
  // tan(NaN) = NaN
  if (ix >= 0x7f800000) {
    return _x - _x;
  }

  Float64 y;
  const Sint32 n{rempio2(_x, y)};
  if (n&1) {
    return tandf<true>(y);
  } else {
    return tandf<false>(y);
  }

  RX_HINT_UNREACHABLE();
}

} // namespace rx::math
