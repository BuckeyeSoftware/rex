#ifndef RX_CORE_RANDOM_MERSENNE_TWISTER_H
#define RX_CORE_RANDOM_MERSENNE_TWISTER_H
#include "rx/core/random/context.h"
#include "rx/core/concurrency/word_lock.h"
namespace Rx::Random {

struct MersenneTwister
  : Context
{
  constexpr MersenneTwister();

  /// Seed the context with |_seed|.
  virtual void seed(Uint64 _seed);

  /// Get a random number in range [0, U32_MAX]
  virtual Uint32 u32();

  /// Get a random number in range [0, U64_MAX]
  virtual Uint64 u64();

  /// Get a random number in range [F32_MIN, F32_MAX]
  virtual Float32 f32();

  /// Get a random number in range [F64_MIN, F64_MAX]
  virtual Float64 f64();

private:
  void generate();

  Uint32 u32_unlocked();
  Uint32 u64_unlocked();

  static inline constexpr const auto SIZE = 624_z;
  static inline constexpr const auto PERIOD = 397_z;
  static inline constexpr const auto DIFFERENCE = SIZE - PERIOD;
  static inline constexpr const auto MAGIC = 0x9908b0df_u32;

  Concurrency::WordLock m_lock;
  union {
    Uint32 m_state[SIZE];
    struct {} m_nat;
  };
  Uint32 m_index;
  bool m_seeded;
};

inline constexpr MersenneTwister::MersenneTwister()
  : m_nat{}
  , m_index{SIZE}
  , m_seeded{false}
{
}

inline Float32 MersenneTwister::f32() {
  return u32() / Float32(0xff'ff'ff'ff_u32);
}

inline Float64 MersenneTwister::f64() {
  return u64() / Float64(0xff'ff'ff'ff'ff'ff'ff'ff_u64);
}

} // namespace Rx::Random

#endif // RX_CORE_RANDOM_MERSENNE_TWISTER_H