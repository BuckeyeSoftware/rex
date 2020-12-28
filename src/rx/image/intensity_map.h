#ifndef RX_IMAGE_INTENSITY_MAP_H
#define RX_IMAGE_INTENSITY_MAP_H
#include "rx/image/matrix.h"
#include "rx/math/vec4.h"

namespace Rx::Image {

struct IntensityMap {
  enum class Mode {
    k_average,
    k_max
  };

  IntensityMap(const Matrix& _image, Mode _mode, const Math::Vec4f& _multiplier);
  const Float32& operator()(Size _x, Size _y) const;

  void invert();

private:
  Matrix m_map;
};

inline const Float32& IntensityMap::operator()(Size _x, Size _y) const {
  return *m_map(_x, _y);
}

} // namespace Rx::Image

#endif // RX_IMAGE_INTENSITY_MAP_H
