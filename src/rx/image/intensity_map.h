#ifndef RX_IMAGE_INTENSITY_MAP_H
#define RX_IMAGE_INTENSITY_MAP_H
#include "rx/image/matrix.h"
#include "rx/math/vec4.h"

namespace rx::image {

struct intensity_map {
  enum class mode {
    k_average,
    k_max
  };

  intensity_map(const matrix& _image, mode _mode, const math::vec4f& _multiplier);
  const rx_f32& operator()(rx_size _x, rx_size _y) const;

  void invert();

private:
  matrix m_map;
};

inline const rx_f32& intensity_map::operator()(rx_size _x, rx_size _y) const {
  return *m_map(_x, _y);
}

} // namespace rx::image

#endif // RX_IMAGE_INTENSITY_MAP_H
