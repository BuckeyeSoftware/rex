#include "rx/core/bitset.h"
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/zero.h"

namespace Rx {

Optional<Bitset> Bitset::create_uninitialized(Memory::Allocator& _allocator, Size _size) {
  const auto words = words_for_size(_size);

  // |words * sizeof(WordType)| would overflow.
  if (words > -1_z / sizeof(WordType)) {
    return nullopt;
  }

  const auto bytes = sizeof(WordType) * words;
  if (auto data = _allocator.allocate(bytes)) {
    Bitset bitset;
    bitset.m_allocator = &_allocator;
    bitset.m_size = _size;
    bitset.m_data = reinterpret_cast<WordType*>(data);
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
  Memory::zero(m_data, words_for_size(m_size));
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
