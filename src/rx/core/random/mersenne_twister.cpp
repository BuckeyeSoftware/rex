#include <time.h> // time(0)

#include "rx/core/random/mersenne_twister.h"
#include "rx/core/concurrency/scope_lock.h"

namespace Rx::Random {

#define M32(_x) (0x80'00'00'00 & (_x)) // 32nd MSB
#define L31(_x) (0x7f'ff'ff'ff & (_x)) // 31 LSBs

#define UNROLL(_expr) \
  do { \
    y = M32(m_state[i]) | L31(m_state[i + 1]); \
    m_state[i] = m_state[(_expr)] ^ (y >> 1) ^ (((Sint32(y) << 31) >> 31) & MAGIC); \
    i++; \
  } while (0)

void MersenneTwister::generate() {
  if (!m_seeded) {
    // TODO(dweiler): Determine a better starter initial seed method.
    seed(5489_u32 + time(0));
    m_seeded = true;
  }

  Size i = 0;
  Uint32 y;

  // i = [0, 225]
  while (i < DIFFERENCE - 1) {
    // 226 = 113*2, even number of steps.
    UNROLL(i + PERIOD);

    // TODO(dweiler): Check why this invokes UB.
    UNROLL(i + PERIOD);
  }

  // i = 226
  UNROLL(i + PERIOD);

  // i = [227, 622]
  while (i < SIZE - 1) {
    // 623 - 227 = 396 = 2*2*3*3*11, can unroll any number that evenly divides.
    UNROLL(i - DIFFERENCE);

    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
    UNROLL(i - DIFFERENCE);
  }

  // i = 623, the last step has to roll over though, so do not use UNROLL
  do {
    y = M32(m_state[SIZE - 1]) | L31(m_state[0]);
    m_state[SIZE - 1] = m_state[PERIOD - 1] ^ (y >> 1) ^ (((Sint32(y) << 31) >> 31) & MAGIC);
  } while (0);

  // Use the tempering method described in the paper.
  for (Size i = 0; i < SIZE; i ++) {
    y = m_state[i];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= y >> 18;
    m_state[i] = y;
  }

  m_index = 0;
}

void MersenneTwister::seed(Uint64 _seed) {
  m_state[0] = _seed;
  for (Size i = 1; i < SIZE; i++) {
    m_state[i] = 0x6c078965 * (m_state[i - 1] ^ m_state[i - 1] >> 30) + i;
  }
}

Uint32 MersenneTwister::u32_unlocked() {
  if (m_index == SIZE) {
    generate();
  }
  return m_state[m_index++];
}

Uint32 MersenneTwister::u64_unlocked() {
  return Uint64(u32_unlocked()) << 32_u64 | u32_unlocked();
}

Uint32 MersenneTwister::u32() {
  Concurrency::ScopeLock lock{m_lock};
  return u32_unlocked();
}

Uint64 MersenneTwister::u64() {
  Concurrency::ScopeLock lock{m_lock};
  return u64_unlocked();
}

} // namespace Rx::Random