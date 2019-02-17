#include <string.h> // memset

#include <rx/core/bitset.h> // bitset
#include <rx/core/assert.h> // RX_ASSERT

rx::bitset::bitset(memory::allocator* alloc, rx_size size)
  : m_allocator{alloc}
  , m_size{size}
  , m_data{m_allocator->allocate(sizeof(bit_type) * (size / k_word_bits + 1))}
{
  RX_ASSERT(m_data, "out of memory");
  memset(m_data.data(), 0, m_data.size());
}
