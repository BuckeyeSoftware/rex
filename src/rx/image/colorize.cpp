#include "rx/image/colorize.h"
#include "rx/image/matrix.h"

#include "rx/color/rgb.h"
#include "rx/color/hsl.h"

namespace Rx::Image {

bool Colorize::process(const Matrix& _src, Matrix& dst_) {
  Color::HSL hsl;
  hsl.h = m_options.hue;
  hsl.s = m_options.saturation;

  auto src = _src.data();
  auto dst = dst_.data();

  auto samples = _src.samples();
  while (samples--) {
    auto luminance = Color::RGB(src->r, src->g, src->b).luminance();
    if (m_options.lightness > 0.0) {
      luminance = luminance * (1.0 - m_options.lightness);
      luminance += 1.0 - (1.0 - m_options.lightness);
    } else {
      luminance = luminance * (m_options.lightness + 1.0);
    }

    hsl.l = luminance;

    const auto rgb = Color::RGB(hsl);
    dst->r = rgb.r;
    dst->g = rgb.g;
    dst->b = rgb.b;
    dst->a = src->a;

    src++;
    dst++;
  }

  return true;
}

} // namespace Rx::Image
