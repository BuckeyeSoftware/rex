#include "rx/image/color_balance.h"
#include "rx/image/matrix.h"

#include "rx/core/algorithm/saturate.h"

#include "rx/color/hsl.h"
#include "rx/color/rgb.h"

namespace Rx::Image {

static inline Float32 map(Float32 _value, Float64 _lightness, Float64 _shadows,
  Float64 _midtones, Float64 _highlights)
{
  using Algorithm::saturate;

  static constexpr const auto A = 0.25;
  static constexpr const auto B = 0.333;
  static constexpr const auto SCALE = 0.7;

  _shadows    *= saturate((_lightness - B) / -A + 0.5) * SCALE;
  _midtones   *= saturate((_lightness - B) /  A + 0.5) *
                 saturate((_lightness + B - 1.0) / -A + 0.5) * SCALE;
  _highlights *= saturate((_lightness + B - 1.0) /  A + 0.5) * SCALE;

  _value += _shadows;
  _value += _midtones;
  _value += _highlights;

  return saturate(_value);
}

bool ColorBalance::process(const Matrix& _src, Matrix& dst_) {
  auto src = _src.data();
  auto dst = dst_.data();

  auto samples = _src.samples();
  while (samples--) {
    auto rgb_src = Color::RGB(src->r, src->g, src->b);
    auto hsl_src = Color::HSL(rgb_src);

    auto rgb_dst = Color::RGB(
      map(rgb_src.r, hsl_src.l, m_options.cr[0], m_options.cr[1], m_options.cr[2]),
      map(rgb_src.g, hsl_src.l, m_options.mg[0], m_options.mg[1], m_options.mg[2]),
      map(rgb_src.b, hsl_src.l, m_options.yb[0], m_options.yb[1], m_options.yb[2]));

    // Use |hsl_src.l| for luma instead of |hsl_dst.l|.
    if (m_options.preserve_luminosity) {
      auto hsl_dst = Color::HSL(rgb_dst);
      hsl_dst = Color::HSL(hsl_dst.h, hsl_dst.s, hsl_src.l, hsl_dst.a);
      rgb_dst = Color::RGB(hsl_dst);
    }

    dst->r = rgb_dst.r;
    dst->g = rgb_dst.g;
    dst->b = rgb_dst.b;
    dst->a = src->a;

    src++;
    dst++;
  }

  return true;
}

} // namespace Rx::Image
