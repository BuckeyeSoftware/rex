#ifndef RX_COLOR_RGB_H
#define RX_COLOR_RGB_H
#include "rx/core/types.h"

#include "rx/core/algorithm/saturate.h"
#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Color {

struct CMYK;
struct HSV;
struct HSL;

struct RGB {
  constexpr RGB();
  constexpr RGB(Float64 _r, Float64 _g, Float64 _b, Float64 _a = 1.0);

  RGB(const CMYK& _cmyk);
  RGB(const HSL& _hsl);
  RGB(const HSV& _hsv);

  static inline constexpr const auto LUMA_R = 0.22248840;
  static inline constexpr const auto LUMA_G = 0.71690369;
  static inline constexpr const auto LUMA_B = 0.06060791;

  constexpr Float64 luminance() const;

  constexpr Float64 min() const;
  constexpr Float64 max() const;

  constexpr RGB saturated() const; // clamped.

  Float64 r;
  Float64 g;
  Float64 b;
  Float64 a;
};

inline constexpr RGB::RGB()
  : RGB{0.0, 0.0, 0.0}
{
}

inline constexpr RGB::RGB(Float64 _r, Float64 _g, Float64 _b, Float64 _a)
  : r{_r}
  , g{_g}
  , b{_b}
  , a{_a}
{
}

inline constexpr Float64 RGB::luminance() const {
  return Algorithm::saturate(r * LUMA_R + g * LUMA_G + b * LUMA_B);
}

inline constexpr Float64 RGB::min() const {
  return Algorithm::min(r, g, b);
}

inline constexpr Float64 RGB::max() const {
  return Algorithm::max(r, g, b);
}

inline constexpr RGB RGB::saturated() const {
  using Algorithm::saturate;
  return {saturate(r), saturate(g), saturate(b), saturate(a)};
}

} // namespace Rx::Color

#endif // RX_COLOR_RGB_H
