#include "rx/core/hash/fnv1a.h"
#include "rx/core/traits/is_same.h"

namespace Rx::Hash {

Uint32 fnv1a_32(const Byte* _data, Size _size) {
  static constexpr const Uint32 PRIME = 0x1000193_u32;
  Uint32 hash = 0x811c9dc5_u32;
  for (Size i = 0; i < _size; i++) {
    hash = hash ^ _data[i];
    hash *= PRIME;
  }
  return hash;
}

Uint64 fnv1a_64(const Byte* _data, Size _size) {
  static constexpr const Uint64 PRIME = 0x100000001b3_u64;
  Uint64 hash = 0xcbf29ce484222325_u64;
  for (Size i = 0; i < _size; i++) {
    hash = hash ^ _data[i];
    hash *= PRIME;
  }
  return hash;
}

} // namespace Rx::Hash
