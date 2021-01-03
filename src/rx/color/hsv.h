#ifndef RX_COLOR_HSV_H
#define RX_COLOR_HSV_H
#include "rx/core/types.h"

#include "rx/core/algorithm/saturate.h"

namespace Rx::Color {

struct RGB;

struct HSV {
  constexpr HSV();
  constexpr HSV(Float64 _h, Float64 _s, Float64 _v, Float64 _a = 1.0);

  HSV(const RGB& _rgb);

  constexpr HSV saturated() const;

  Float64 h;
  Float64 s;
  Float64 v;
  Float64 a;
};

inline constexpr HSV::HSV()
  : HSV{0.0, 0.0, 0.0}
{
}

inline constexpr HSV::HSV(Float64 _h, Float64 _s, Float64 _v, Float64 _a)
  : h{_h}
  , s{_s}
  , v{_v}
  , a{_a}
{
}

inline constexpr HSV HSV::saturated() const {
  using Algorithm::saturate;

  auto f = h - Sint32(h);
  if (f < 0.0) {
    f += 1.0;
  }

  return {f, saturate(s), saturate(v), saturate(a)};
}

} // namespace Rx::Color

#endif // RX_COLOR_HSV_H
