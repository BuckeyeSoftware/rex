#include "rx/core/prng/mt19937.h"
#include "rx/core/hints/unlikely.h"

namespace Rx::PRNG {

static inline constexpr Uint32 m32(Uint32 _x) {
  return 0x80000000 & _x;
}

static inline constexpr Uint32 l31(Uint32 _x) {
  return 0x7fffffff & _x;
}

static inline constexpr bool odd(Uint32 _x) {
  return _x & 1;
}

void MT19937::seed(Uint32 _seed) {
  m_index = 0;
  m_state[0] = _seed;
  for (Size i = 1; i < SIZE; i++) {
    m_state[i] = 0x6c078965 * (m_state[i - 1] ^ m_state[i - 1] >> 30) + i;
  }
}

Uint32 MT19937::u32() {
  m_lock.lock();

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

  m_lock.unlock();

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

} // namespace Rx::PRNG
