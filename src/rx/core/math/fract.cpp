#include <float.h> // {DBL,LDBL}_EPSILON

#include "rx/core/math/fract.h"
#include "rx/core/math/floor.h"

namespace Rx::Math {

Float64 fract(Float64 _x) {
  return _x - floor(_x);
}

Float32 fract(Float32 _x) {
  return _x - floor(_x);
}

} // namespace Rx::Math
