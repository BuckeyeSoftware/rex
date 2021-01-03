#ifndef RX_COLOR_HSL_H
#define RX_COLOR_HSL_H
#include "rx/core/types.h"

namespace Rx::Color {

struct RGB;

struct HSL {
  constexpr HSL();
  constexpr HSL(Float64 _h, Float64 _s, Float64 _l, Float64 _a = 1.0);

  HSL(const RGB& _rgb);

  Float64 h;
  Float64 s;
  Float64 l;
  Float64 a;
};

inline constexpr HSL::HSL()
  : HSL{0.0, 0.0, 0.0}
{
}

inline constexpr HSL::HSL(Float64 _h, Float64 _s, Float64 _l, Float64 _a)
  : h{_h}
  , s{_s}
  , l{_l}
  , a{_a}
{
}

} // namespace Rx::Color

#endif // RX_COLOR_HSL_H
