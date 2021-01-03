#include "rx/color/cmyk.h"
#include "rx/color/transform.h"

namespace Rx::Color {

CMYK::CMYK(const RGB& _rgb) {
  rgb_to_cmyk(_rgb, *this);
}

} // namespace Rx::Color
