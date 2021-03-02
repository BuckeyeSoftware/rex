#include <string.h> // memset

#include "rx/core/bitset.h"
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/memory/system_allocator.h"

namespace Rx {

Optional<Bitset> Bitset::create_uninitialized(Memory::Allocator& _allocator, Size _size) {
  auto bytes = bytes_for_size(_size);
  if (auto data = _allocator.allocate(bytes)) {
    Bitset bitset;
    bitset.m_allocator = &_allocator;
    bitset.m_size = _size;
    bitset.m_data = reinterpret_cast<BitType*>(data);
    return bitset;
  }
  return nullopt;
}

Optional<Bitset> Bitset::create(Memory::Allocator& _allocator, Size _size) {
  if (auto bitset = create_uninitialized(_allocator, _size)) {
    bitset->clear();
    return bitset;
  }
  return nullopt;
}

void Bitset::release() {
  if (m_allocator) {
    m_allocator->deallocate(m_data);
  }
}

void Bitset::clear() {
  memset(m_data, 0, bytes_for_size(m_size));
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
