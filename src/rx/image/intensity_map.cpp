#include "rx/image/intensity_map.h"

namespace rx::image {

intensity_map::intensity_map(const matrix& _image, mode _mode, const math::vec4f& _multiplier)
  : m_map{_image.allocator(), _image.dimensions(), 1}
{
  const rx_size channels{_image.channels()};

  const math::vec2z& dimensions{m_map.dimensions()};
  for (rx_size y{0}; y < dimensions.h; y++) {
    for (rx_size x{0}; x < dimensions.w; x++) {
      math::vec4f rgba;
      const rx_f32* pixel{_image(x, y)};
      for (rx_size channel{0}; channel < channels; channel++) {
        rgba[channel] = pixel[channel] * _multiplier[channel];
      }

      switch (_mode) {
      case mode::k_average:
        *m_map(x, y) = rgba.sum() / channels;
        break;
      case mode::k_max:
        *m_map(x, y) = rgba.max_element();
        break;
      }
    }
  }
}

void intensity_map::invert() {
  const math::vec2z& dimensions{m_map.dimensions()};
  for (rx_size y{0}; y < dimensions.h; y++) {
    for (rx_size x{0}; x < dimensions.w; x++) {
      *m_map(x, y) = 1.0f - *m_map(x, y);
    }
  }
}

} // namespace rx::image
