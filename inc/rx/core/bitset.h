#ifndef RX_CORE_BITSET_H
#define RX_CORE_BITSET_H

#include <limits.h> // CHAR_BIT

#include <rx/core/utility/move.h>

#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/memory/allocator.h> // memory::{allocator, block}

namespace rx {

// 32-bit: 16 bytes
// 64-bit: 32 bytes
struct bitset {
  using bit_type = rx_u64;

  static constexpr const bit_type k_bit_one{1};
  static constexpr const rx_size k_word_bits{CHAR_BIT * sizeof(bit_type)};

  bitset(rx_size size);
  bitset(memory::allocator* alloc, rx_size size);
  bitset(bitset&& set);
  ~bitset();

  void set(rx_size bit);
  void clear(rx_size bit);
  void clear_all();
  bool test(rx_size bit) const;
  rx_size size() const;

  rx_size find_first_set() const;
  rx_size find_first_unset() const;

private:
  static rx_size index(rx_size bit);
  static rx_size offset(rx_size bit);

  memory::allocator* m_allocator;
  rx_size m_size;
  memory::block m_data;
};

inline bitset::bitset(bitset&& set)
  : m_allocator{set.m_allocator}
  , m_data{utility::move(set.m_data)}
{
  set.m_allocator = nullptr;
}

inline bitset::~bitset() {
  if (m_allocator) {
    m_allocator->deallocate(utility::move(m_data));
  }
}

inline void bitset::set(rx_size bit) {
  RX_ASSERT(bit < m_size, "out of bounds");
  m_data.cast<bit_type*>()[index(bit)] |= k_bit_one << offset(bit);
}

inline void bitset::clear(rx_size bit) {
  RX_ASSERT(bit < m_size, "out of bounds");
  m_data.cast<bit_type*>()[index(bit)] &= ~(k_bit_one << offset(bit));
}

inline bool bitset::test(rx_size bit) const {
  RX_ASSERT(bit < m_size, "out of bounds");
  return !!(m_data.cast<const bit_type*>()[index(bit)] & (k_bit_one << offset(bit)));
}

inline rx_size bitset::size() const {
  return m_size;
}

inline rx_size bitset::index(rx_size bit) {
  return bit / k_word_bits;
}

inline rx_size bitset::offset(rx_size bit) {
  return bit % k_word_bits;
}

} // namespace rx

#endif // RX_CORE_BITSET_H
