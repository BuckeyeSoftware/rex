#include "rx/core/math/abs.h"
#include "rx/core/math/shape.h"

namespace Rx::Math {

Float64 abs(Float64 _x) {
  Shape u{_x};
  u.as_u64 &= -1_u64 / 2;
  return u.as_f64;
}

Float32 abs(Float32 _x) {
  Shape u{_x};
  u.as_u32 &= -1_u32 / 2;
  return u.as_f32;
}

} // namespace Rx::Math
