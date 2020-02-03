#include "rx/image/convert.h"
#include "rx/image/matrix.h"

namespace rx::image {

bool convert(const matrix& _matrix, vector<rx_byte>& data_) {
  const math::vec2z& dimensions{_matrix.dimensions()};
  const rx_size channels{_matrix.channels()};

  if (!data_.resize(dimensions.area() * _matrix.channels(), utility::uninitialized{})) {
    return false;
  }

  for (rx_size y{0}; y < dimensions.h; y++) {
    for (rx_size x{0}; x < dimensions.w; x++) {
      for (rx_size channel{0}; channel < channels; channel++) {
        const rx_size index{(y * dimensions.w + x) * channels + channel};
        data_[index] = static_cast<rx_byte>(_matrix[index] * 255.0f);
      }
    }
  }

  return true;
}

bool convert(const rx_byte* _data, const math::vec2z& _dimensions,
  rx_size _channels, matrix& matrix_)
{
  if (!matrix_.resize(_dimensions, _channels)) {
    return false;
  }

  for (rx_size y{0}; y < _dimensions.h; y++) {
    for (rx_size x{0}; x < _dimensions.w; x++) {
      for (rx_size channel{0}; channel < _channels; channel++) {
        const rx_size index{(y * _dimensions.w + x) * _channels + channel};
        matrix_[index] = static_cast<rx_f32>(_data[index]) / 255.0f;
      }
    }
  }

  return true;
}

} // namespace rx::image
