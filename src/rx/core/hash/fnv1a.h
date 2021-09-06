#ifndef RX_CORE_HASH_FNV1A_H
#define RX_CORE_HASH_FNV1A_H
#include "rx/core/types.h"

/// \file fnv1a.h
///
/// Fowler-Noll-Vo hash algorithm.

namespace Rx::Hash {

/// @{
/// Hash memory contents and produce a 32-bit or 64-bit hash value.
/// \param _data Pointer to memory to hash.
/// \param _size Number of bytes to read and hash from \p _data.
RX_API Uint32 fnv1a_32(const Byte* _data, Size _size);
RX_API Uint64 fnv1a_64(const Byte* _data, Size _size);
/// @}

/// @{
/// Hash null-terminated string and produce a 32-bit or 64-bit hash value.
/// These interfaces ignore the null-terminator and are faster than the raw
/// variants for when the length of the string is not known, as computing the
/// string length has non-constant cost.
///
/// \param _data The string to hash.
RX_API Uint32 fnv1a_str_32(const char* _data);
RX_API Uint64 fnv1a_str_64(const char* _data);
/// @}

} // namespace Rx::Hash

#endif // RX_CORE_HASH_FNV1A_H
