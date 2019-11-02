#include "rx/core/math/tan.h"
#include "rx/math/constants.h"

#include "rx/image/brightness_contrast.h"
#include "rx/image/matrix.h"

namespace rx::image {

static rx_f32 map(rx_f32 _value, rx_f32 _brightness, rx_f32 _slant) {
  if (_brightness < 0.0f) {
    _value = _value * (1.0f + _brightness);
  } else {
    _value = _value + ((1.0f - _value) * _brightness);
  }

  _value = (_value - 0.5f) * _slant + 0.5f;

  return _value;
}

void brightness_contrast::operator()(matrix& matrix_) const {
  const rx_f32 slant{math::tan((contrast + 1.0f) * math::k_pi<rx_f32> * 4.0f)};

  for (rx_size i{0}; i < matrix_.samples(); i++) {
    matrix_[i + 0] = map(matrix_[i + 0], brightness / 2.0f, slant);
    matrix_[i + 1] = map(matrix_[i + 1], brightness / 2.0f, slant);
    matrix_[i + 2] = map(matrix_[i + 2], brightness / 2.0f, slant);
  }
}

} // namespace rx::image
