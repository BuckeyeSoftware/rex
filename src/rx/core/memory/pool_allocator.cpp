#include <string.h> // memset

#include <rx/core/utility/move.h>

#include <rx/core/memory/allocator.h> // allocator
#include <rx/core/memory/pool_allocator.h> // pool_allocator

#include <rx/core/debug.h> // mesg

namespace rx::memory {

pool_allocator::pool_allocator(allocator* alloc, rx_size size, rx_size capacity)
  : m_allocator{alloc}
  , m_size{size}
  , m_data{utility::move(m_allocator->allocate(m_size * capacity))}
  , m_bits{alloc, capacity}
{
  RX_ASSERT(m_data, "out of memory");
}

pool_allocator::~pool_allocator() {
  for (rx_size i{0}; i < m_bits.size(); i++) {
    if (m_bits.test(i)) {
      RX_MESSAGE("leaked object %zu %p in pool", i, data_of(i));
    }
  }
  m_allocator->deallocate(utility::move(m_data));
}

rx_byte* pool_allocator::allocate() {
  const rx_size bit{m_bits.find_first_unset()};
  if (bit != -1_z) {
    m_bits.set(bit);
    return data_of(bit);
  }
  return nullptr;
}

void pool_allocator::deallocate(rx_byte* _data) {
  if (_data) {
    m_bits.clear(index_of(_data));
  }
}

} // namespace rx::memory
