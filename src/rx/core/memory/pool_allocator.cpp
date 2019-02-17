#include <string.h> // memset

#include <rx/core/traits.h> // move
#include <rx/core/memory/allocator.h> // allocator
#include <rx/core/memory/pool_allocator.h> // pool_allocator

namespace rx::memory {

pool_allocator::pool_allocator(allocator* alloc, rx_size size, rx_size capacity)
  : m_allocator{alloc}
  , m_size{size}
  , m_data{move(m_allocator->allocate(m_size * capacity))}
  , m_bits{alloc, capacity}
{
  RX_ASSERT(m_data, "out of memory");
}

pool_allocator::~pool_allocator() {
  for (rx_size i{0}; i < m_bits.size(); i++) {
    RX_ASSERT(!m_bits.test(i), "leaked object in pool");
  }
  m_allocator->deallocate(move(m_data));
}

block pool_allocator::allocate() {
  for (rx_size i{0}; i < m_bits.size(); i++) {
    if (m_bits.test(i)) {
      continue;
    }
    m_bits.set(i);
    return {m_size, m_data.data() + m_size * i};
  }
  return {};
}

void pool_allocator::deallocate(block&& data) {
  if (data) {
    m_bits.clear(index_of(data));
  }
}

} // namespace rx::memory
