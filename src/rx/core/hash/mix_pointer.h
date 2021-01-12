#ifndef RX_CORE_HASH_MIX_POINTER_H
#define RX_CORE_HASH_MIX_POINTER_H
#include "rx/core/hash/mix_int.h"

namespace Rx::Hash {

template<typename T>
inline constexpr Size mix_pointer(const T* _pointer) {
  if constexpr (sizeof _pointer == 8) {
    return mix_uint64(reinterpret_cast<Uint64>(_pointer));
  } else {
    return mix_uint32(reinterpret_cast<Uint32>(_pointer));
  }
}

} // namespace Rx::Hash

#endif // RX_CORE_MIX_POINTER_H
