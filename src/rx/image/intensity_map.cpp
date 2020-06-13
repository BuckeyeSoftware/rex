#include "rx/image/intensity_map.h"

namespace Rx::Image {

IntensityMap::IntensityMap(const Matrix& _image, Mode _mode, const Math::Vec4f& _multiplier)
  : m_map{_image.allocator(), _image.dimensions(), 1}
{
  const Size channels{_image.channels()};

  const Math::Vec2z& dimensions{m_map.dimensions()};
  for (Size y{0}; y < dimensions.h; y++) {
    for (Size x{0}; x < dimensions.w; x++) {
      Math::Vec4f rgba;
      const Float32* pixel{_image(x, y)};
      for (Size channel{0}; channel < channels; channel++) {
        rgba[channel] = pixel[channel] * _multiplier[channel];
      }

      switch (_mode) {
      case Mode::k_average:
        *m_map(x, y) = rgba.sum() / channels;
        break;
      case Mode::k_max:
        *m_map(x, y) = rgba.max_element();
        break;
      }
    }
  }
}

void IntensityMap::invert() {
  const Math::Vec2z& dimensions{m_map.dimensions()};
  for (Size y{0}; y < dimensions.h; y++) {
    for (Size x{0}; x < dimensions.w; x++) {
      *m_map(x, y) = 1.0f - *m_map(x, y);
    }
  }
}

} // namespace rx::image
