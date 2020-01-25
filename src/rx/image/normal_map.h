#ifndef RX_IMAGE_NORMAL_MAP_H
#define RX_IMAGE_NORMAL_MAP_H
#include "rx/image/intensity_map.h"

namespace rx::image {

struct normal_map {
  enum class kernel {
    k_sobel,
    k_prewitt
  };

  enum {
    k_invert = 1 << 0,
    k_tile   = 1 << 1,
    k_detail = 1 << 2
  };

  normal_map(const matrix& _image);
  normal_map(matrix&& image_);

  matrix generate(intensity_map::mode _mode, const math::vec4f& _multiplier,
    kernel _kernel, rx_f32 _strength, int _flags,
    rx_f32 _detail = 0.0f) const;

private:
  matrix m_image;
};

inline normal_map::normal_map(const matrix& _image)
  : m_image{_image}
{
}

inline normal_map::normal_map(matrix&& image_)
  : m_image{utility::move(image_)}
{
}

} // namespace rx::image

#endif // RX_IMAGE_NORMAL_MAP_H
