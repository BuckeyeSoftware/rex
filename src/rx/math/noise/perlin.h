#ifndef RX_MATH_NOISE_PERLIN_H
#define RX_MATH_NOISE_PERLIN_H
#include "rx/core/types.h"

namespace Rx::Random {
  struct Context;
}

namespace Rx::Math::Noise {

struct Perlin {
  Perlin(Random::Context& _context);

  Float32 noise(Float32 _x) const;
  Float32 noise(Float32 _x, Float32 _y) const;
  Float32 noise(Float32 _x, Float32 _y, Float32 _z) const;

  void reseed();

private:
  Random::Context& m_random;
  Byte m_data[512];
};

inline Float32 Perlin::noise(Float32 _x) const {
  return noise(_x, 0.0f, 0.0f);
}

inline Float32 Perlin::noise(Float32 _x, Float32 _y) const {
  return noise(_x, _y, 0.0f);
}

} // namespace Rx::Math::noise

#endif // RX_MATH_NOISE_PERLIN_H
