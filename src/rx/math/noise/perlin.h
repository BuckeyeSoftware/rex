#ifndef RX_MATH_NOISE_PERLIN_H
#define RX_MATH_NOISE_PERLIN_H
#include "rx/core/types.h"

namespace rx::prng {
  struct mt19937;
}

namespace rx::math::noise {

struct perlin {
  perlin(prng::mt19937& _mt19937);

  rx_f32 noise(rx_f32 _x) const;
  rx_f32 noise(rx_f32 _x, rx_f32 _y) const;
  rx_f32 noise(rx_f32 _x, rx_f32 _y, rx_f32 _z) const;

  void reseed();

private:
  prng::mt19937& m_mt19937;
  rx_byte m_data[512];
};

inline rx_f32 perlin::noise(rx_f32 _x) const {
  return noise(_x, 0.0f, 0.0f);
}

inline rx_f32 perlin::noise(rx_f32 _x, rx_f32 _y) const {
  return noise(_x, _y, 0.0f);
}

} // namespace rx::math::noise

#endif // RX_MATH_NOISE_PERLIN_H
