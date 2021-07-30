#ifndef RX_CORE_HASH_FNV1A_H
#define RX_CORE_HASH_FNV1A_H
#include "rx/core/types.h"

// # Fowler-Noll-Vo hash

namespace Rx::Hash {

RX_API Uint32 fnv1a_32(const Byte* _data, Size _size);
RX_API Uint64 fnv1a_64(const Byte* _data, Size _size);

RX_API Uint32 fnv1a_str_32(const char* _data);
RX_API Uint64 fnv1a_str_64(const char* _data);

} // namespace Rx::Hash

#endif // RX_CORE_HASH_FNV1A_H
