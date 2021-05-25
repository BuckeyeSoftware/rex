#include "rx/core/random/mt19937.h"
#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/hints/unlikely.h"

namespace Rx::Random {

static inline constexpr Uint32 m32(Uint32 _x) {
  return 0x80000000 & _x;
}

static inline constexpr Uint32 l31(Uint32 _x) {
  return 0x7fffffff & _x;
}

static inline constexpr bool odd(Uint32 _x) {
  return _x & 1;
}

void MT19937::seed(Uint64 _seed) {
  m_index = 0;
  m_state[0] = _seed;
  for (Size i = 1; i < SIZE; i++) {
    m_state[i] = 0x6c078965 * (m_state[i - 1] ^ m_state[i - 1] >> 30) + i;
  }
}

Uint32 MT19937::u32_unlocked() {
  if (RX_HINT_UNLIKELY(m_index == 0)) {
    generate();
  }

  Uint32 value = m_state[m_index];

  value ^= value >> 11;
  value ^= value << 7 & 0x9d2c5680;
  value ^= value << 15 & 0xefc60000;
  value ^= value >> 18;

  if (RX_HINT_UNLIKELY(++m_index == SIZE)) {
    m_index = 0;
  }

  return value;
}

void MT19937::generate() {
  Uint32 i = 0;
  Uint32 y = 0;

  auto unroll = [&](Uint32 _expr) {
    y = m32(m_state[i]) | l31(m_state[i+1]);
    m_state[i] = m_state[_expr] ^ (y >> 1) ^ (0x9908b0df * odd(y));
    i++;
  };

  // i = [0, 226]
  while (i < DIFFERENCE - 1) {
    unroll(i + PERIOD);
    unroll(i + PERIOD);
  }

  // i = 256
  unroll((i + PERIOD) % SIZE);

  // i = [227, 622]
  while (i < SIZE - 1) {
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
    unroll(i - DIFFERENCE);
  }

  // i = 623
  y = m32(m_state[SIZE - 1]) | l31(m_state[0]);
  m_state[SIZE - 1] = m_state[PERIOD - 1] ^ (y >> 1) ^ (0x9908b0df * odd(y));
}

Uint32 MT19937::u32() {
  Concurrency::ScopeLock lock{m_lock};
  return u32_unlocked();
}

Uint64 MT19937::u64() {
  Concurrency::ScopeLock lock{m_lock};
  const auto a = u32_unlocked();
  const auto b = u32_unlocked();
  return static_cast<Uint64>(a) << 32_u64 | b;
}

Float32 MT19937::f32() {
  return static_cast<Float32>(u32()) / Float32(MAX);
}

Float64 MT19937::f64() {
  return static_cast<Float64>(u64()) / Float64(MAX);
}

} // namespace Rx::Random
