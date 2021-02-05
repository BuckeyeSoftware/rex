#ifndef RX_CORE_PRNG_MT19937_H
#define RX_CORE_PRNG_MT19937_H
#include "rx/core/types.h"

namespace Rx::PRNG {

struct RX_API MT19937 {
  constexpr MT19937();

  void seed(Uint32 _seed);

  Uint32 u32();
  Uint64 u64();

  Float32 f32();
  Float64 f64();

private:
  static inline constexpr const auto SIZE = 624_z;
  static inline constexpr const auto PERIOD = 397_z;
  static inline constexpr const auto DIFFERENCE = SIZE - PERIOD;
  static inline constexpr const auto MAX = 0xffffffff_u32;

  void generate();

  union {
    struct {} m_nat;
    Uint32 m_state[SIZE];
  };
  Size m_index;
};

inline constexpr MT19937::MT19937()
  : m_nat{}
  , m_index{0}
{
}

inline Uint64 MT19937::u64() {
  return static_cast<Uint64>(u32()) << 32_u64 | u32();
}

inline Float32 MT19937::f32() {
  return static_cast<Float32>(u32()) / Float64(MAX);
}

inline Float64 MT19937::f64() {
  return static_cast<Float64>(u32()) / Float64(MAX);
}

}

#endif // RX_CORE_PRNG_MT19937_H
