#include "rx/color/transform.h"
#include "rx/color/cmyk.h"
#include "rx/color/hsl.h"
#include "rx/color/hsv.h"
#include "rx/color/rgb.h"

#include "rx/core/math/constants.h"

namespace Rx::Color {

void rgb_to_hsv(const RGB& _rgb, HSV& hsv_) {
  const auto min = _rgb.min();
  const auto max = _rgb.max();

  hsv_.v = max;

  if (const auto delta = max - min; delta > Math::EPSILON<Float64>) {
    hsv_.s = delta / max;
    if (_rgb.r == max) {
      hsv_.h = (_rgb.g - _rgb.b) / delta;
      if (hsv_.h < 0.0) {
        hsv_.h += 6.0;
      }
    } else if (_rgb.g == max) {
      hsv_.h = 2.0 + (_rgb.b - _rgb.r) / delta;
    } else {
      hsv_.h = 4.0 + (_rgb.r - _rgb.g) / delta;
    }

    hsv_.h /= 6.0;
  } else {
    hsv_.s = 0.0;
    hsv_.h = 0.0;
  }

  hsv_.a = _rgb.a;
}

void hsv_to_rgb(const HSV& _hsv, RGB& rgb_) {
  auto h = _hsv.h;
  auto s = _hsv.s;
  auto v = _hsv.v;

  if (s == 0.0) {
    rgb_.r = v;
    rgb_.g = v;
    rgb_.b = v;
  } else {
    if (h == 1.0) {
      h = 0.0;
    }

    h *= 6.0;

    const auto i = Sint32(h);
    const auto f = h - i;
    const auto w = v * (1.0 - _hsv.s);
    const auto q = v * (1.0 - (_hsv.s * f));
    const auto t = v * (1.0 - (_hsv.s * (1.0 - f)));

    switch (i) {
    case 0: rgb_.r = v, rgb_.g = t, rgb_.b = w; break;
    case 1: rgb_.r = q, rgb_.g = v, rgb_.b = w; break;
    case 2: rgb_.r = w, rgb_.g = v, rgb_.b = t; break;
    case 3: rgb_.r = w, rgb_.g = q, rgb_.b = v; break;
    case 4: rgb_.r = t, rgb_.g = w, rgb_.b = v; break;
    case 5: rgb_.r = v, rgb_.g = w, rgb_.b = q; break;
    }
  }

  rgb_.a = _hsv.a;
}

void rgb_to_hsl(const RGB& _rgb, HSL& hsl_) {
  const auto min = _rgb.min();
  const auto max = _rgb.max();

  hsl_.l = (max + min) / 2.0;

  if (max == min) {
    hsl_.s = 0.0;
    hsl_.h = -1.0;
  } else {
    auto delta = max - min;
    if (hsl_.l <= 0.5) {
      hsl_.s = delta / (max + min);
    } else {
      hsl_.s = delta / (2.0 - max - min);
    }

    if (delta == 0.0) {
      delta = 1.0;
    }

    if (_rgb.r == max) {
      hsl_.h = (_rgb.g - _rgb.b) / delta;
    } else if (_rgb.g == max) {
      hsl_.h = 2.0 + (_rgb.b - _rgb.r) / delta;
    } else {
      hsl_.h = 4.0 + (_rgb.r - _rgb.g) / delta;
    }

    hsl_.h /= 6.0;

    if (hsl_.h < 0.0) {
      hsl_.h += 1.0;
    }
  }

  hsl_.a = _rgb.a;
}

static inline Float64 hsl_map(Float64 _n1, Float64 _n2, Float64 _hue) {
  if (_hue > 6.0) {
    _hue -= 6.0;
  } else if (_hue < 0.0) {
    _hue += 6.0;
  }

  if (_hue < 1.0) {
    return _n1 + (_n2 - _n1) * _hue;
  } else if (_hue < 3.0) {
    return _n2;
  } else if (_hue < 4.0) {
    return _n1 + (_n2 - _n1) * (4.0 - _hue);
  }

  return _n1;
}

void hsl_to_rgb(const HSL& _hsl, RGB& rgb_) {
  const auto h = _hsl.h;
  const auto l = _hsl.l;
  const auto s = _hsl.s;

  if (s == 0.0) {
    rgb_.r = l;
    rgb_.g = l;
    rgb_.b = l;
  } else {
    auto m2 = l <= 0.5 ? l * (1.0 + s) : l + s - l * s;
    auto m1 = 2.0 * l - m2;

    rgb_.r = hsl_map(m1, m2, h * 6.0 + 2.0);
    rgb_.g = hsl_map(m1, m2, h * 6.0);
    rgb_.b = hsl_map(m1, m2, h * 6.0 - 2.0);
  }
}

void rgb_to_cmyk(const RGB& _rgb, CMYK& cmyk_) {
  // TODO(dweiler): Custom black pullout derivation.
  auto p = 1.0;

  const auto c = 1.0 - _rgb.r;
  const auto m = 1.0 - _rgb.g;
  const auto y = 1.0 - _rgb.b;

  auto k = 1.0;
  if (c < k) k = c;
  if (m < k) k = m;
  if (y < k) k = y;
  k *= p;

  if (k < 1.0) {
    cmyk_.c = (c - k) / (1.0 - k);
    cmyk_.m = (m - k) / (1.0 - k);
    cmyk_.y = (y - k) / (1.0 - k);
  } else {
    cmyk_.c = 0.0;
    cmyk_.m = 0.0;
    cmyk_.y = 0.0;
  }

  cmyk_.k = k;
  cmyk_.a = _rgb.a;
}

void cmyk_to_rgb(const CMYK& _cmyk, RGB& rgb_) {
  if (const auto k = _cmyk.k; k < 1.0) {
    rgb_.r = 1.0 - (_cmyk.c * (1.0 - k) + k);
    rgb_.g = 1.0 - (_cmyk.m * (1.0 - k) + k);
    rgb_.b = 1.0 - (_cmyk.y * (1.0 - k) + k);
  } else {
    rgb_.r = 0.0;
    rgb_.g = 0.0;
    rgb_.b = 0.0;
  }
  rgb_.a = _cmyk.a;
}

} // namespace Rx::Color
