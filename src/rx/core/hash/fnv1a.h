#ifndef RX_CORE_HASH_FNV1A_H
#define RX_CORE_HASH_FNV1A_H
#include "rx/core/types.h"

// # Fowler-Noll-Vo hash

namespace rx::hash {

template<typename T>
T fnv1a(const rx_byte* _data, rx_size _size);

} // namespace rx::hash

#endif // RX_CORE_HASH_FNV1A_H
