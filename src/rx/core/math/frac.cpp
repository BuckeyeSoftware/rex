#include <float.h> // {DBL,LDBL}_EPSILON

#include "rx/core/math/frac.h"
#include "rx/core/math/floor.h"

namespace Rx::Math {

Float64 frac(Float64 _x) {
  return _x - floor(_x);
}

Float32 frac(Float32 _x) {
  return _x - floor(_x);
}

} // namespace Rx::Math
