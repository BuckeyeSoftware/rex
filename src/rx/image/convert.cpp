#include "rx/image/convert.h"
#include "rx/image/matrix.h"

namespace Rx::Image {

bool convert(const Matrix& _matrix, LinearBuffer& data_) {
  const Math::Vec2z& dimensions{_matrix.dimensions()};
  const Size channels{_matrix.channels()};

  if (!data_.resize(dimensions.area() * _matrix.channels())) {
    return false;
  }

  for (Size y{0}; y < dimensions.h; y++) {
    for (Size x{0}; x < dimensions.w; x++) {
      for (Size channel{0}; channel < channels; channel++) {
        const Size index{(y * dimensions.w + x) * channels + channel};
        data_[index] = static_cast<Byte>(_matrix[index] * 255.0f);
      }
    }
  }

  return true;
}

bool convert(const Byte* _data, const Math::Vec2z& _dimensions, Size _channels,
  Matrix& matrix_)
{
  if (!matrix_.resize(_dimensions, _channels)) {
    return false;
  }

  for (Size y{0}; y < _dimensions.h; y++) {
    for (Size x{0}; x < _dimensions.w; x++) {
      for (Size channel{0}; channel < _channels; channel++) {
        const Size index{(y * _dimensions.w + x) * _channels + channel};
        matrix_[index] = static_cast<Float32>(_data[index]) / 255.0f;
      }
    }
  }

  return true;
}

} // namespace Rx::Image
