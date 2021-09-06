#ifndef RX_CORE_HASH_COMBINE_H
#define RX_CORE_HASH_COMBINE_H
#include "rx/core/types.h" // Size
#include "rx/core/array.h"

/// \file combine.h
/// This is an implementation of the TEA algorithm.
///
/// Phi = (1 + sqrt(5)) / 2 = 1.6180339887498948482045868343656
///
/// Where 2^n / phi is used as the "magic constant" truncated to n bits.
///
/// Computing this for 32 bit as an example:
///  2^32 / Phi = 6544357694972302964775847707926
///             = 0x9e3779b9
namespace Rx::Hash {

/// Combine two 16-bit hashes into one.
inline constexpr Uint16 combine_u16(Uint16 _hash1, Uint16 _hash2) {
  return _hash1 ^ (_hash2 + 0x9e37_u16 + (_hash1 << 3) + (_hash1 >> 1));
}

/// Combine two 32-bit hashes into one.
inline constexpr Uint32 combine_u32(Uint32 _hash1, Uint32 _hash2) {
  return _hash1 ^ (_hash2 + 0x9e3779b9_u32 + (_hash1 << 6) + (_hash1 >> 2));
}

/// Combine two 64-bit hashes into one.
inline constexpr Uint64 combine_u64(Uint64 _hash1, Uint64 _hash2) {
  return _hash1 ^ (_hash2 + 0x9e3779b97f4a7c15_u64 + (_hash1 << 12) + (_hash1 >> 4));
}

/// Combine two Size hashes into one.
inline constexpr Size combine(Size _hash1, Size _hash2) {
  if constexpr (sizeof(Size) == 8) {
    return combine_u64(_hash1, _hash2);
  } else {
    return combine_u32(_hash1, _hash2);
  }
}

/// Combine two djbx33a hashes into one.
RX_API Array<Byte[16]> combine(const Array<Byte[16]>& _hash1, const Array<Byte[16]>& _hash2);

} // namespace Rx::Hash

#endif // RX_CORE_HASH_COMBINE_H
