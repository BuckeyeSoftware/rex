#include "rx/color/rgb.h"
#include "rx/color/transform.h"

namespace Rx::Color {

RGB::RGB(const CMYK& _cmyk) {
  cmyk_to_rgb(_cmyk, *this);
}

RGB::RGB(const HSL& _hsl) {
  hsl_to_rgb(_hsl, *this);
}

RGB::RGB(const HSV& _hsv) {
  hsv_to_rgb(_hsv, *this);
}

} // namespace Rx::Color
