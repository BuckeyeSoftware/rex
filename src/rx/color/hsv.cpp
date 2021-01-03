#include "rx/color/hsv.h"
#include "rx/color/transform.h"

namespace Rx::Color {

HSV::HSV(const RGB& _rgb) {
  rgb_to_hsv(_rgb, *this);
}

} // namespace Rx::Color
