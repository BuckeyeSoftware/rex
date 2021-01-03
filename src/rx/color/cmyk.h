#ifndef RX_COLOR_CMYK_H
#define RX_COLOR_CMYK_H
#include "rx/core/types.h"

namespace Rx::Color {

struct RGB;

struct CMYK {
  constexpr CMYK();
  constexpr CMYK(Float64 _c, Float64 _m, Float64 _y, Float64 _k, Float64 _a = 1.0);

  CMYK(const RGB& _rgb);

  Float64 c;
  Float64 m;
  Float64 y;
  Float64 k;
  Float64 a;
};

inline constexpr CMYK::CMYK()
  : CMYK{0.0, 0.0, 0.0, 0.0}
{
}

inline constexpr CMYK::CMYK(Float64 _c, Float64 _m, Float64 _y, Float64 _k, Float64 _a)
  : c{_c}
  , m{_m}
  , y{_y}
  , k{_k}
  , a{_a}
{
}

} // namespace Rx::Color

#endif // RX_COLOR_CMYK_H
