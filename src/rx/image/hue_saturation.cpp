#include "rx/image/hue_saturation.h"
#include "rx/image/matrix.h"

#include "rx/color/rgb.h"
#include "rx/color/hsl.h"

namespace Rx::Image {

using Algorithm::saturate;

static inline Float64 map_hue(
  const HueSaturation::Options& _options,
  Float64 _value,
  Sint32 _range)
{
  _value += (_options.hue[HueSaturation::ALL] + _options.hue[_range]) / 2.0;
  if (_value < 0.0) {
    return _value + 1.0;
  } else if (_value > 1.0) {
    return _value - 1.0;
  } else {
    return _value;
  }
}

static inline Float64 map_hue_overlap(
  const HueSaturation::Options& _options,
  Float64 _value,
  Sint32 _p_range,
  Float64 _p_intensity,
  Sint32 _s_range,
  Float64 _s_intensity)
{
  auto v = _options.hue[_p_range] * _p_intensity +
           _options.hue[_s_range] * _s_intensity;

  _value += (_options.hue[HueSaturation::ALL] + v) / 2.0;

  if (_value < 0.0) {
    return _value + 1.0;
  } else if (_value > 1.0) {
    return _value - 1.0;
  } else {
    return _value;
  }
}

static inline Float64 map_saturation(
  const HueSaturation::Options& _options,
  Float64 _value,
  Sint32 _range)
{
  auto v = _options.saturation[HueSaturation::ALL] + _options.saturation[_range];
  return saturate(_value * (v + 1.0));
}

static inline Float64 map_lightness(
  const HueSaturation::Options& _options,
  Float64 _value,
  Sint32 _range)
{
  auto v = _options.lightness[HueSaturation::ALL] + _options.lightness[_range];
  if (v < 0.0) {
    return _value * (v + 1.0);
  } else {
    return _value + (v * (1.0 - _value));
  }
}

bool HueSaturation::process(const Matrix& _src, Matrix& dst_) {
  auto src = _src.data();
  auto dst = dst_.data();

  auto samples = _src.samples();

  auto overlap = m_options.overlap / 2.0;

  while (samples--) {
    // Create RGB and convert to HSL.
    auto rgb = Color::RGB(src->r, src->g, src->b, src->a);
    auto hsl = Color::HSL(rgb);

    // Determine primary and secondary hue and intensity.
    auto p_hue = 0;
    auto s_hue = 0;
    auto p_intensity = 0.0f;
    auto s_intensity = 0.0f;

    auto h = hsl.h * 6.0;

    auto use_secondary_hue = false;
    for (auto c = 0; c < 7; c++) {
      auto hue_threshold = Float64(c) + 0.5;
      if (h < hue_threshold + overlap) {
        p_hue = c;
        if (overlap > 0.0 && h > hue_threshold - overlap) {
          use_secondary_hue = true;
          s_hue = c + 1;
          s_intensity = (h - hue_threshold + overlap) / (2.0 * overlap);
          p_intensity = 1.0 - s_intensity;
        } else {
          use_secondary_hue = false;
        }
        break;
      }
    }

    if (p_hue >= 6) {
      p_hue = 0;
      use_secondary_hue = false;
    }

    if (s_hue >= 6) {
      s_hue = 0;
    }

    p_hue++;
    s_hue++;

    if (use_secondary_hue) {
      hsl.h = map_hue_overlap(m_options, hsl.h, p_hue, p_intensity, s_hue, s_intensity);
      hsl.s = map_saturation(m_options, hsl.s, p_hue) * p_intensity +
              map_saturation(m_options, hsl.s, s_hue) * s_intensity;
      hsl.l = map_lightness(m_options, hsl.l, p_hue) * p_intensity +
              map_lightness(m_options, hsl.l, s_hue) * s_intensity;
    } else {
      hsl.h = map_hue(m_options, hsl.h, p_hue);
      hsl.s = map_saturation(m_options, hsl.s, p_hue);
      hsl.l = map_lightness(m_options, hsl.l, p_hue);
    }

    // Convert back to RGB.
    rgb = Color::RGB(hsl);

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
