#ifndef RX_CORE_HASH_STRING_H
#define RX_CORE_HASH_STRING_H
#include "rx/core/hash/fnv1a.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Hash {

inline Size string(const char* _string) {
  if constexpr (sizeof(Size) == 8) {
    return fnv1a_str_64(_string);
  } else {
    return fnv1a_str_32(_string);
  }
  RX_HINT_UNREACHABLE();
}

inline Size string(const char* _string, Size _length) {
  if constexpr (sizeof(Size) == 8) {
    return fnv1a_64(reinterpret_cast<const Byte*>(_string), _length);
  } else {
    return fnv1a_32(reinterpret_cast<const Byte*>(_string), _length);
  }
  RX_HINT_UNREACHABLE();
}

} // namespace Rx::Hash

#endif // RX_CORE_HASH_STRING_H