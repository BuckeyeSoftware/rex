#ifndef RX_CORE_HASH_FNV1A_H
#define RX_CORE_HASH_FNV1A_H
#include "rx/core/types.h"

// # Fowler-Noll-Vo hash

namespace Rx::Hash {

Uint32 fnv1a_32(const Byte* _data, Size _size);
Uint64 fnv1a_64(const Byte* _data, Size _size);

Uint32 fnv1a_str_32(const char* _data);
Uint64 fnv1a_str_64(const char* _data);

} // namespace Rx::Hash

#endif // RX_CORE_HASH_FNV1A_H
