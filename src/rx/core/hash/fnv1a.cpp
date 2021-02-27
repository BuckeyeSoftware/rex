#include "rx/core/hash/fnv1a.h"
#include "rx/core/traits/is_same.h"

namespace Rx::Hash {

static constexpr const Uint32 PRIME_32 = 0x1000193_u32;
static constexpr const Uint32 SEED_32 = 0x811c9dc5_u32;

static constexpr const Uint64 PRIME_64 = 0x100000001b3_u64;
static constexpr const Uint64 SEED_64 = 0xcbf29ce484222325_u64;

Uint32 fnv1a_32(const Byte* _data, Size _size) {
  Uint32 hash = SEED_32;
  for (Size i = 0; i < _size; i++) {
    hash = hash ^ _data[i];
    hash *= PRIME_32;
  }
  return hash;
}

Uint64 fnv1a_64(const Byte* _data, Size _size) {
  Uint64 hash = SEED_64;
  for (Size i = 0; i < _size; i++) {
    hash = hash ^ _data[i];
    hash *= PRIME_64;
  }
  return hash;
}

Uint32 fnv1a_str_32(const char* _data) {
  Uint32 hash = SEED_32;
  for (const char* ch = _data; *ch; ch++) {
    hash = hash ^ Byte(*ch);
    hash *= PRIME_32;
  }
  return hash;
}

Uint64 fnv1a_str_64(const char* _data) {
  Uint64 hash = SEED_64;
  for (const char* ch = _data; *ch; ch++) {
    hash = hash ^ Byte(*ch);
    hash *= PRIME_64;
  }
  return hash;
}

} // namespace Rx::Hash
