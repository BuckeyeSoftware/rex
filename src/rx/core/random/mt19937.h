#ifndef RX_CORE_RANDOM_MT19937_H
#define RX_CORE_RANDOM_MT19937_H
#include "rx/core/random/context.h"
#include "rx/core/concurrency/word_lock.h"

namespace Rx::Random {

struct RX_API MT19937
  : Context
{
  constexpr MT19937();

  virtual void seed(Uint64 _seed);

  virtual Uint32 u32();
  virtual Uint64 u64();

  virtual Float32 f32();
  virtual Float64 f64();

private:
  static inline constexpr const auto SIZE = 624_z;
  static inline constexpr const auto PERIOD = 397_z;
  static inline constexpr const auto DIFFERENCE = SIZE - PERIOD;
  static inline constexpr const auto MAX = 0xffffffff_u32;

  void generate();

  Uint32 u32_unlocked();

  Uint32 m_state[SIZE];
  Uint32 m_index;
  Concurrency::WordLock m_lock;
};

inline constexpr MT19937::MT19937()
  : m_state{0}
  , m_index{0}
{
}

}

#endif // RX_CORE_RANDOM_MT19937_H
