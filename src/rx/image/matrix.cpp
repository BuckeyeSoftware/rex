#include "rx/image/matrix.h"

namespace rx::image {

matrix matrix::scaled(const math::vec2z& _dimensions) const {
  RX_ASSERT(_dimensions != m_dimensions, "cannot scale to same size");

  const rx_size channels{m_channels};

  // Really slow Bilinear scaler.
  matrix result{m_allocator, _dimensions, channels};

  const rx_f32 x_ratio = rx_f32(m_dimensions.w - 1) / _dimensions.w;
  const rx_f32 y_ratio = rx_f32(m_dimensions.h - 1) / _dimensions.h;

  for (rx_size y{0}; y < _dimensions.h; y++) {
    for (rx_size x{0}; x < _dimensions.w; x++) {
      const auto x_index{static_cast<rx_s32>(x_ratio * x)};
      const auto y_index{static_cast<rx_s32>(y_ratio * y)};

      const rx_f32 x_diff{(x_ratio * x) - x_index};
      const rx_f32 y_diff{(y_ratio * y) - y_index};

      for (rx_size channel{0}; channel < channels; channel++) {
        const rx_size index{y_index * m_dimensions.w * channels + x_index * channels + channel};

        const rx_f32 a{m_data[index]};
        const rx_f32 b{m_data[index + channels]};
        const rx_f32 c{m_data[index + m_dimensions.w*channels]};
        const rx_f32 d{m_data[index + m_dimensions.w*channels + channels]};

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

} // namespace rx::image
