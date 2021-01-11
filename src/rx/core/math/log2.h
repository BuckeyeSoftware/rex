#ifndef RX_CORE_MATH_LOG2_H
#define RX_CORE_MATH_LOG2_H
#include "rx/core/types.h"
#include "rx/core/traits/is_same.h"

namespace Rx::Math {

// Use compiler intrinsics if available.
#if defined(RX_COMPILER_CLANG) || defined(RX_COMPILER_GCC)
inline Uint32 log2(Uint32 _value) {
  static_assert(Traits::IS_SAME<Uint32, unsigned int>, "Uint32 not unsigned int");
  return sizeof _value * 8 - __builtin_clz(_value) - 1;
}

inline Uint64 log2(Uint64 _value) {
  if constexpr (Traits::IS_SAME<Uint64, unsigned long>) {
    return sizeof _value * 8 - __builtin_clzl(_value) - 1;
  } else {
    return sizeof _value * 8 - __builtin_clzll(_value) - 1;
  }
}
#else
inline Uint32 log2(Uint32 _value) {
  static constexpr const Byte TABLE[] = {
    0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12,
    20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
  };
  _value |= _value >> 1;
  _value |= _value >> 2;
  _value |= _value >> 4;
  _value |= _value >> 8;
  _value |= _value >> 16;
  return TABLE[Uint32(_value * 0x07c4acdd) >> 27];
}

inline Uint64 log2(Uint64 _value) {
  static constexpr const Byte TABLE[] = {
    63, 0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61, 51,
    37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62, 57, 46,
    52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31,
    35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5
  };
  _value |= _value >> 1;
  _value |= _value >> 2;
  _value |= _value >> 4;
  _value |= _value >> 8;
  _value |= _value >> 16;
  _value |= _value >> 32;
  return TABLE[(Uint64((_value - (_value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
}
#endif

} // namespace Rx::Math

#endif // RX_MATH_LOG2_H
