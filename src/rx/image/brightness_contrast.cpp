#include "rx/image/brightness_contrast.h"
#include "rx/image/matrix.h"

#include "rx/core/math/tan.h"
#include "rx/core/math/constants.h"

namespace Rx::Image {

static inline Float32 map(Float32 _value, Float64 _brightness, Float64 _slant) {
  if (_brightness < 0.0) {
    _value = _value * (1.0 + _brightness);
  } else {
    _value = _value + ((1.0 - _value) * _brightness);
  }

  _value = (_value - 0.5) * _slant + 0.5;

  return _value;
}

bool BrightnessContrast::process(const Matrix& _src, Matrix& dst_) {
  const auto brightness = m_options.brightness / 2.0;
  const auto slant = Math::tan((m_options.contrast + 1.0) * Math::PI_4<Float64>);

  auto src = _src.data();
  auto dst = dst_.data();

  auto samples = _src.samples();
  while (samples--) {
    dst->r = map(src->r, brightness, slant);
    dst->g = map(src->g, brightness, slant);
    dst->b = map(src->b, brightness, slant);
    dst->a = src->a;
    src++;
    dst++;
  }

  return true;
}

} // namespace Rx::Image
