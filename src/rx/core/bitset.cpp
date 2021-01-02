#include <string.h> // memset

#include "rx/core/bitset.h"
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/memory/system_allocator.h"

namespace Rx {

Optional<Bitset> Bitset::create(Memory::Allocator& _allocator, Size _size) {
  auto bytes = bytes_for_size(_size);
  if (auto data = _allocator.allocate(bytes)) {
    memset(data, 0, bytes);
    return Bitset{_allocator, _size, reinterpret_cast<BitType*>(data)};
  }
  return nullopt;
}

Optional<Bitset> Bitset::copy(Memory::Allocator& _allocator, const Bitset& _bitset) {
  auto size = _bitset.m_size;
  auto bytes = bytes_for_size(size);
  if (auto data = _allocator.allocate(bytes_for_size(bytes))) {
    memcpy(data, _bitset.m_data, bytes);
    return Bitset{_allocator, size, reinterpret_cast<BitType*>(data)};
  }
  return nullopt;
}

Bitset& Bitset::operator=(Bitset&& bitset_) {
  RX_ASSERT(&bitset_ != this, "self assignment");

  m_allocator = &bitset_.allocator();
  m_size = Utility::exchange(bitset_.m_size, 0);
  m_data = Utility::exchange(bitset_.m_data, nullptr);

  return *this;
}

Size Bitset::count_set_bits() const {
  Size count = 0;

  for (Size i = 0; i < m_size; i++) {
    if (test(i)) {
      count++;
    }
  }

  return count;
}

Size Bitset::count_unset_bits() const {
  Size count = 0;

  for (Size i = 0; i < m_size; i++) {
    if (!test(i)) {
      count++;
    }
  }

  return count;
}

Optional<Size> Bitset::find_first_unset() const {
  for (Size i = 0; i < m_size; i++) {
    if (!test(i)) {
      return i;
    }
  }
  return nullopt;
}

Optional<Size> Bitset::find_first_set() const {
  for (Size i = 0; i < m_size; i++) {
    if (test(i)) {
      return i;
    }
  }
  return nullopt;
}

} // namespace Rx
