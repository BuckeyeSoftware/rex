#ifndef RX_IMAGE_NORMAL_MAP_H
#define RX_IMAGE_NORMAL_MAP_H
#include "rx/image/intensity_map.h"

namespace Rx::Image {

struct NormalMap {
  enum class Kernel {
    k_sobel,
    k_prewitt
  };

  enum {
    k_invert = 1 << 0,
    k_tile   = 1 << 1,
    k_detail = 1 << 2
  };

  NormalMap(const Matrix& _image);
  NormalMap(Matrix&& image_);

  Matrix generate(IntensityMap::Mode _mode, const Math::Vec4f& _multiplier,
                  Kernel _kernel, Float32 _strength, int _flags,
                  Float32 _detail = 0.0f) const;

private:
  Matrix m_image;
};

inline NormalMap::NormalMap(const Matrix& _image)
  : m_image{_image}
{
}

inline NormalMap::NormalMap(Matrix&& image_)
  : m_image{Utility::move(image_)}
{
}

} // namespace Rx::Image

#endif // RX_IMAGE_NORMAL_MAP_H
