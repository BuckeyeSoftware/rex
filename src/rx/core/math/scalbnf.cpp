#include "rx/core/math/scalbnf.h"
#include "rx/core/math/shape.h"

namespace Rx::Math {

Float32 scalbnf(Float32 _x, Sint32 _n) {
  Float32Eval y = _x;

  if (_n > 127) {
    y *= 0x1p127f;
    _n -= 127;
    if (_n > 127) {
      y *= 0x1p127f;
      _n -= 127;
      if (_n > 127) {
        _n = 127;
      }
    }
  } else if (_n < -126) {
    y *= 0x1p-126f * 0x1p24f;
    _n += 126 - 24;
    if (_n < -126) {
      y *= 0x1p-126f * 0x1p24f;
      _n += 126 - 24;
      if (_n < -126) {
        _n = -126;
      }
    }
  }

  return y * Shape{Uint32(0x7f + _n) << 23}.as_f32;
}

} // namespace Rx::Math
