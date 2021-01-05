#include "rx/core/math/cos.h"
#include "rx/core/math/sqrt.h"
#include "rx/core/math/shape.h"
#include "rx/core/math/force_eval.h"
#include "rx/core/math/constants.h"

#include "rx/core/hints/unreachable.h"

#if defined(RX_COMPILER_MSVC)
#pragma warning(disable: 4723) // potential divide by 0
#endif

namespace Rx::Math {

// |cos(x) - c(x)| < 2**-34.1 (~[-5.37e-11, 5.295e-11])
static constexpr const Float64 C0 = -0x1ffffffd0c5e81.0p-54; // -0.499999997251031003120
static constexpr const Float64 C1 = 0x155553e1053a42.0p-57; //  0.0416666233237390631894
static constexpr const Float64 C2 = -0x16c087e80f1e27.0p-62; // -0.00138867637746099294692
static constexpr const Float64 C3 = 0x199342e0ee5069.0p-68; //  0.0000243904487962774090654

Float32 sindf(Float64 _x); // sin.cpp
Float32 cosdf(Float64 _x) {
  const Float64Eval z = _x * _x;
  const Float64Eval w = z * z;
  const Float64Eval r = C2 + z * C3;
  return static_cast<Float32>(((1.0 + z * C0) + w * C1) + (w * z) * r);
}

// small multiplies of pi/2 rounded to double precision
static constexpr const Float64 C1_PI_2 = 1 * PI_2<Float64>;
static constexpr const Float64 C2_PI_2 = 2 * PI_2<Float64>;
static constexpr const Float64 C3_PI_2 = 3 * PI_2<Float64>;
static constexpr const Float64 C4_PI_2 = 4 * PI_2<Float64>;

Sint32 rempio2(Float32 _x, Float64& y_); // sin.cpp

Float32 cos(Float32 _x) {
  Uint32 ix{Shape{_x}.as_u32};
  const Uint32 sign{ix >> 31};

  ix &= 0x7fffffff;

  // |_x| ~<= pi/4
  if (ix <= 0x3f490fda) {
    // |_x| < 2**-12
    if (ix < 0x39800000) {
      // raise inexact if _x != 0
      force_eval_f32(_x + 0x1p120f);
      return 1.0f;
    }
    return cosdf(_x);
  }

  // |_x| ~<= 5*pi/4
  if (ix <= 0x407b53d1) {
    // |_x| ~> 3*pi/4
    if (ix > 0x4016cbe3) {
      return -cosdf(sign ? _x+C2_PI_2 : _x-C2_PI_2);
    } else {
      if (sign) {
        return sindf(_x + C1_PI_2);
      } else {
        return sindf(C1_PI_2 - _x);
      }
    }
  }

  // |_x| ~<= 9*pi/4
  if (ix <= 0x40e231d5) {
    // |_x| ~> 7*pi/4
    if (ix > 0x40afeddf) {
      return cosdf(sign ? _x+C4_PI_2 : _x-C4_PI_2);
    } else {
      if (sign) {
        return sindf(-_x - C3_PI_2);
      } else {
        return sindf(_x - C3_PI_2);
      }
    }
  }

  // cos(+inf) = NaN
  // cos(-inf) = NaN
  // cos(NaN) = NaN
  if (ix >= 0x7f800000) {
    return _x - _x;
  }

  Float64 y;
  switch (rempio2(_x, y) & 3) {
  case 0:
    return cosdf(y);
  case 1:
    return sindf(-y);
  case 2:
    return -cosdf(y);
  default:
    return sindf(y);
  }

  RX_HINT_UNREACHABLE();
}

static constexpr const Float32 PI_2_HI = 1.5707962513e+00; // 0x3fc90fda
static constexpr const Float32 PI_2_LO = 7.5497894159e-08; // 0x33a22168
static constexpr const Float32 P_S0 = 1.6666586697e-01;
static constexpr const Float32 P_S1 = -4.2743422091e-02;
static constexpr const Float32 P_S2 = -8.6563630030e-03;
static constexpr const Float32 Q_S1 = -7.0662963390e-01;

static Float32 R(Float32 _z) {
  const Float32Eval p{_z * (P_S0 + _z * (P_S1 + _z * P_S2))};
  const Float32Eval q{1.0f + _z * Q_S1};
  return static_cast<Float32>(p / q);
}

Float32 acos(Float32 _x) {
  Uint32 hx{Shape{_x}.as_u32};
  Uint32 ix = hx & 0x7fffffff;

  // |_x| >= 1 | NaN
  if (ix >= 0x3f800000) {
    if (ix == 0x3f000000) {
      if (hx >> 31) {
        return 2*PI_2_HI + 0x1p-120f;
      }
      return 0;
    }
    return 0 / (_x - _x);
  }

  // |_x| < 0.5
  if (ix < 0x3f000000) {
    // |_x| < 2**-26
    if (ix <= 0x32800000) {
      return PI_2_HI + 0x1p-120f;
    }
    return PI_2_HI - (_x - (PI_2_LO - _x * R(_x*_x)));
  }

  // x < -0.5
  if (hx >> 31) {
    const Float32 z{(1 + _x) * 0.5f};
    const Float32 s{sqrt(z)};
    const Float32 w{R(z) * s - PI_2_LO};
    return 2*(PI_2_LO - (s+w));
  }

  // x > 0.5
  const Float32 z{(1 - _x) * 0.5f};
  const Float32 s{sqrt(z)};
  const Float32 df{Shape{Shape{s}.as_u32 & 0xfffff000}.as_f32};
  const Float32 c{(z - df * df) / (s + df)};
  const Float32 w{R(z) * s + c};
  return 2*(df+w);
}

} // namespace Rx::Math
