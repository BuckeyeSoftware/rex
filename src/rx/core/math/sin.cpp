#define _USE_MATH_DEFINES
#include <float.h> // {DBL,LDBL}_EPSILON

#include "rx/core/math/abs.h"
#include "rx/core/math/sin.h"
#include "rx/core/math/sqrt.h"
#include "rx/core/math/shape.h"
#include "rx/core/math/force_eval.h"
#include "rx/core/math/constants.h"

#include "rx/core/abort.h"
#include "rx/core/hints/unreachable.h"

namespace Rx::Math {

// |sin(x)/x - s(x)| < 2**-37.5 (~[-4.89e-12, 4.824e-12])
static constexpr const Float64 S1 = -0x15555554cbac77.0p-55; // -0.166666666416265235595
static constexpr const Float64 S2 = 0x111110896efbb2.0p-59;  //  0.0083333293858894631756
static constexpr const Float64 S3 = -0x1a00f9e2cae774.0p-65; // -0.000198393348360966317347
static constexpr const Float64 S4 = 0x16cd878c3b46a7.0p-71;  //  0.0000027183114939898219064

Float32 cosdf(Float64 _x); // cos.cpp
Float32 sindf(Float64 _x) {
  const Float64Eval z = _x * _x;
  const Float64Eval w = z * z;
  const Float64Eval r = S3 + z * S4;
  const Float64Eval s = z * _x;
  return static_cast<Float32>((_x + s * (S1 + z * S2)) + s * w * r);
}


#if RX_FLOAT_EVAL_METHOD == 0 || RX_FLOAT_EVAL_METHOD == 1
static constexpr const Float64 TO_INT = 1.5 / DBL_EPSILON;
#else
static constexpr const Float64 TO_INT = 1.5 / LDBL_EPSILON;
#endif

static constexpr const Float64 INVPIO2 = 6.36619772367581382433e-01;
static constexpr const Float64 PIO2_1  = 1.57079631090164184570e+00;
static constexpr const Float64 PIO2_1T = 1.58932547735281966916e-08;

Sint32 rempio2(Float32 _x, Float64& y_) {
  const Shape u{_x};
  const Uint32 ix{u.as_u32 & 0x7fffffff};

  // |_x| ~< 2^28*(pi/2)
  if (ix < 0x4dc90fdb) {
    const auto fn{static_cast<Float64Eval>(_x) * INVPIO2 + TO_INT - TO_INT};
    const auto n{static_cast<Sint32>(fn)};
    y_ = _x - fn*PIO2_1 - fn*PIO2_1T;
    return n;
  }

  // x is inf or NaN
  if (ix >= 0x7f800000) {
    y_ = _x - _x;
    return 0;
  }

  Rx::abort("range error");
}

// small multiplies of pi/2 rounded to double precision
static constexpr Float64 S1_PI_2 = 1 * PI_2<Float64>;
static constexpr Float64 S2_PI_2 = 2 * PI_2<Float64>;
static constexpr Float64 S3_PI_2 = 3 * PI_2<Float64>;
static constexpr Float64 S4_PI_2 = 4 * PI_2<Float64>;

Float32 sin(Float32 _x) {
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
    return sindf(_x);
  }

  // |_x| ~<= 5*pi/4
  if (ix <= 0x407b53d1) {
    // |_x| ~<= 3*pi/4
    if (ix <= 0x4016cbe3) {
      if (sign) {
        return -cosdf(_x + S1_PI_2);
      } else {
        return cosdf(_x - S1_PI_2);
      }
    }
    return sindf(sign ? -(_x + S2_PI_2) : -(_x - S2_PI_2));
  }

  // |_x| ~<= 9*pi/4
  if (ix <= 0x40e231d5) {
    // |_x| ~<= 7*pi/4
    if (ix <= 0x40afeddf) {
      if (sign) {
        return cosdf(_x + S3_PI_2);
      } else {
        return -cosdf(_x - S3_PI_2);
      }
    }
    return sindf(sign ? _x + S4_PI_2 : _x - S4_PI_2);
  }

  // sin(+inf) = NaN
  // sin(-inf) = NaN
  // sin(NaN) = NaN
  if (ix >= 0x7f800000) {
    return _x - _x;
  }

  Float64 y{0};
  switch (rempio2(_x, y) & 3) {
  case 0:
    return sindf(y);
  case 1:
    return cosdf(y);
  case 2:
    return sindf(-y);
  default:
    return -cosdf(y);
  }

  RX_HINT_UNREACHABLE();
}

// Coefficients for R(x^2).
static constexpr Float64 PS0 =  1.6666586697e-01;
static constexpr Float64 PS1 = -4.2743422091e-02;
static constexpr Float64 PS2 = -8.6563630030e-03;
static constexpr Float64 QS1 = -7.0662963390e-01;

static Float32 R(Float32 _z) {
  const Float32Eval p = _z * (PS0 + _z * (PS1 + _z * PS2));
  const Float32Eval q = 1.0f + _z * QS1;
  return p / q;
}

static constexpr const Float64 PIO2 = 1.570796326794896558e+00;

Float32 asin(Float32 _x) {
  auto hx = Shape{_x}.as_u32;
  auto ix = hx & 0x7fffffff;

  // |_x| >= 1
  if (ix >= 0x3f800000) {
    // |_x| == 1|
    if (ix == 0x3f800000) {
      // asin(+-1) = +-pi/2 with inexact.
      return _x * PIO2 + 0x1p-120f;
    }
    // asin(|x|>1) is NaN
    return 0/(_x - _x);
  }

  // |_x| < 0.5
  if (ix < 0x3f000000) {
    // if 0x1p-126 <= |_x| < 0x1p-12, avoid raising underflow.
    if (ix < 0x39800000 && ix >= 0x00800000) {
      return _x;
    }

    return _x + _x * R(_x * _x);
  }

  // 1 > |x| >= 0.5
  const Float32 z = (1.0f - abs(_x)) * 0.5f;
  const Float32 s = sqrt(z);
  const Float32 x = PIO2 - 2.0 * (s + s * R(z));

  return hx >> 31 ? -x : x;
}

} // namespace Rx::Math
