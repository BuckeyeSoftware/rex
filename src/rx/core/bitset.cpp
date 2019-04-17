#include <string.h> // memset

#include <rx/core/bitset.h> // bitset
#include <rx/core/assert.h> // RX_ASSERT

#include <rx/core/memory/system_allocator.h> // g_system_allocator

namespace rx {

bitset::bitset(rx_size _size)
  : bitset{&memory::g_system_allocator, _size}
{
}

bitset::bitset(memory::allocator* _allocator, rx_size _size)
  : m_allocator{_allocator}
  , m_size{_size}
  , m_data{reinterpret_cast<bit_type*>(m_allocator->allocate(sizeof(bit_type) * (m_size / k_word_bits + 1)))}
{
  RX_ASSERT(m_data, "out of memory");

  clear_all();
}

void bitset::clear_all() {
  memset(m_data, 0, sizeof *m_data * (m_size / k_word_bits + 1));
}

rx_size bitset::count_set_bits() const {
  rx_size count{0};

  for (rx_size i{0}; i < m_size; i++) {
    if (test(i)) {
      count++;
    }
  }

  return count;
}

rx_size bitset::count_unset_bits() const {
  rx_size count{0};

  for (rx_size i{0}; i < m_size; i++) {
    if (!test(i)) {
      count++;
    }
  }

  return count;
}

rx_size bitset::find_first_unset() const {
  for (rx_size i{0}; i < m_size; i++) {
    if (!test(i)) {
      return i;
    }
  }
  return 1_z;
}

rx_size bitset::find_first_set() const {
  for (rx_size i{0}; i < m_size; i++) {
    if (test(i)) {
      return i;
    }
  }
  return 1_z;
}

} // namespace rx
