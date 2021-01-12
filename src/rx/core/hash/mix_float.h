#ifndef RX_CORE_HASH_MIX_FLOAT_H
#define RX_CORE_HASH_MIX_FLOAT_H
#include "rx/core/math/shape.h"
#include "rx/core/hash/mix_int.h"
#include "rx/core/concepts/floating_point.h"

namespace Rx::Hash {

inline constexpr Size mix_float32(Float32 _value) {
  return mix_uint32(Math::Shape{_value}.as_u32);
}

inline constexpr Size mix_float64(Float64 _value) {
  return mix_uint64(Math::Shape{_value}.as_u64);
}

template<Concepts::FloatingPoint T>
inline constexpr Size mix_float(T _value) {
  if constexpr (Traits::IS_SAME<T, Float32>) {
    return mix_float32(_value);
  } else if constexpr (Traits::IS_SAME<T, Float64>) {
    return mix_float64(_value);
  }
}

} // namespace Rx::Hash

#endif // RX_CORE_HASH_MIX_FLOAT_H
