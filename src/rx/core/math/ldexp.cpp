#include "rx/core/math/ldexp.h"
#include "rx/core/math/scalbnf.h"

namespace Rx::Math {

Float32 ldexp(Float32 _x, Sint32 _n) {
  return scalbnf(_x, _n);
}

} // namespace Rx::Math
