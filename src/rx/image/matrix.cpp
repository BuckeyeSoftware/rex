#include "rx/image/matrix.h"

namespace Rx::Image {

Matrix Matrix::scaled(const Math::Vec2z& _dimensions) const {
  RX_ASSERT(_dimensions != m_dimensions, "cannot scale to same size");

  const Size channels{m_channels};

  // Really slow Bilinear scaler.
  Matrix result{allocator(), _dimensions, channels};

  const Float32 x_ratio = Float32(m_dimensions.w - 1) / _dimensions.w;
  const Float32 y_ratio = Float32(m_dimensions.h - 1) / _dimensions.h;

  for (Size y{0}; y < _dimensions.h; y++) {
    for (Size x{0}; x < _dimensions.w; x++) {
      const auto x_index{static_cast<Sint32>(x_ratio * x)};
      const auto y_index{static_cast<Sint32>(y_ratio * y)};

      const Float32 x_diff{(x_ratio * x) - x_index};
      const Float32 y_diff{(y_ratio * y) - y_index};

      for (Size channel{0}; channel < channels; channel++) {
        const Size index{y_index * m_dimensions.w * channels + x_index * channels + channel};

        const Float32 a{data()[index]};
        const Float32 b{data()[index + channels]};
        const Float32 c{data()[index + m_dimensions.w * channels]};
        const Float32 d{data()[index + m_dimensions.w * channels + channels]};

        result[y * _dimensions.w * channels + x * channels + channel] =
           a * (1.0f - x_diff) * (1.0f - y_diff) +
           b * (       x_diff) * (1.0f - y_diff) +
           c * (       y_diff) * (1.0f - x_diff) +
           d * (       x_diff) * (       y_diff);
      }
    }
  }

  return result;
}

} // namespace Rx::Image
