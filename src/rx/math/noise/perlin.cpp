#include "rx/math/noise/perlin.h"
#include "rx/core/math/floor.h"
#include "rx/core/prng/mt19937.h"
#include "rx/core/utility/swap.h"

namespace rx::math::noise {

static inline rx_f32 fade(rx_f32 _t) {
  return _t * _t * _t * (_t * (_t * 6.0f - 15.0f) + 10.0f);
}

static inline rx_f32 lerp(rx_f32 _t, rx_f32 _a, rx_f32 _b) {
  return _a + _t * (_b - _a);
}

static inline rx_f32 grad(rx_byte _hash, rx_f32 _x, rx_f32 _y, rx_f32 _z) {
  const rx_byte hash{static_cast<rx_byte>(_hash & 15)};
  const rx_f32 u{hash < 5 ? _x : _y};
  const rx_f32 v{hash < 4 ? _y : hash == 12 || hash == 14 ? _x : _z};
  return ((_hash & 1) == 0 ? u : -u) + ((hash & 2) == 0 ? v : -v);
}

perlin::perlin(prng::mt19937& _mt19937)
  : m_mt19937{_mt19937}
{
  reseed();
}

void perlin::reseed() {
  for (rx_size i{0}; i < 256; i++) {
    m_data[i] = static_cast<rx_byte>(i);
  }

  rx_size n{256};
  for (rx_size i{0}; i < n - 1; i++) {
    const rx_size j{m_mt19937.u32() % (i + 1)};
    utility::swap(m_data[i], m_data[j]);
  }

  for (rx_size i{0}; i < 256; i++) {
    m_data[256 + i] = m_data[i];
  }
}

rx_f32 perlin::noise(rx_f32 _x, rx_f32 _y, rx_f32 _z) const {
  const rx_s32 X{static_cast<rx_s32>(floor(_x)) & 255};
  const rx_s32 Y{static_cast<rx_s32>(floor(_y)) & 255};
  const rx_s32 Z{static_cast<rx_s32>(floor(_z)) & 255};

  _x -= floor(_x);
  _y -= floor(_y);
  _z -= floor(_z);

  const rx_f32 u{fade(_x)};
  const rx_f32 v{fade(_y)};
  const rx_f32 w{fade(_z)};

  const rx_s32 A{m_data[X] + Y};
  const rx_s32 AA{m_data[A] + Z};
  const rx_s32 AB{m_data[A + 1] + Z};

  const rx_s32 B{m_data[X + 1] + Y};
  const rx_s32 BA{m_data[B] + Z};
  const rx_s32 BB{m_data[B + 1] + Z};

  return lerp(w, lerp(v, lerp(u, grad(m_data[AA],     _x,        _y,        _z),
                                 grad(m_data[BA],     _x - 1.0f, _y,        _z)),
                         lerp(u, grad(m_data[AB],     _x,        _y - 1.0f, _z),
                                 grad(m_data[BB],     _x - 1.0f, _y - 1.0f, _z))),
                 lerp(v, lerp(u, grad(m_data[AA + 1], _x,        _y,        _z - 1.0f),
                                 grad(m_data[BA + 1], _x - 1.0f, _y,        _z - 1.0f)),
                         lerp(u, grad(m_data[AB + 1], _x,        _y - 1.0f, _z - 1.0f),
                                 grad(m_data[BB + 1], _x - 1.0f, _y - 1.0f, _z - 1.0f))));
}

} // namespace rx::math::noise
