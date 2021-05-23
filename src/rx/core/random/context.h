#ifndef RX_CORE_RANDOM_CONTEXT_H
#define RX_CORE_RANDOM_CONTEXT_H
#include "rx/core/types.h"

namespace Rx::Random {

struct Context {
  virtual ~Context() = default;

  /// Seed the context with |_seed|.
  virtual void seed(Uint64 _seed) = 0;

  /// Get a random number in range [0, U32_MAX]
  virtual Uint32 u32() = 0;

  /// Get a random number in range [0, U64_MAX]
  virtual Uint64 u64() = 0;

  /// Get a random number in range [F32_MIN, F32_MAX]
  virtual Float32 f32() = 0;

  /// Get a random number in range [F64_MIN, F64_MAX]
  virtual Float64 f64() = 0;
};

} // namespace Rx::Random

#endif // RX_CORE_RANDOM_CONTEXT_H