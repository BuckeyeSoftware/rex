#include "rx/core/hash/fnv1a.h"
#include "rx/core/traits/is_same.h"

namespace rx::hash {

template<typename T>
T fnv1a(const rx_byte* _data, rx_size _size) {
  if constexpr (traits::is_same<T, rx_u32>) {
    static constexpr const rx_u32 k_prime = 0x1000193_u32;
    rx_u32 hash = 0x811c9dc5_u32;
    for (rx_size i = 0; i < _size; i++) {
      hash = hash ^ _data[i];
      hash *= k_prime;
    }
  } else if constexpr (traits::is_same<T, rx_u64>) {
    static constexpr const rx_u64 k_prime = 0x100000001b3_u64;
    rx_u64 hash = 0xcbf29ce484222325_u64;
    for (rx_size i = 0; i < _size; i++) {
      hash = hash ^ _data[i];
      hash *= k_prime;
    }
    return hash;
  }
  return 0;
}

template rx_u32 fnv1a<rx_u32>(const rx_byte* _data, rx_size _size);
template rx_u64 fnv1a<rx_u64>(const rx_byte* _data, rx_size _size);

} // namespace rx::hash
