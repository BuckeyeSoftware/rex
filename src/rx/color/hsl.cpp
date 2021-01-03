#include "rx/color/hsl.h"
#include "rx/color/transform.h"

namespace Rx::Color {

HSL::HSL(const RGB& _rgb) {
  rgb_to_hsl(_rgb, *this);
}

} // namespace Rx::Color
