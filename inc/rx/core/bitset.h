#ifndef RX_CORE_BITSET_H
#define RX_CORE_BITSET_H

#include <limits.h> // CHAR_BIT

#include <rx/core/utility/move.h>

#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/memory/allocator.h> // memory::{allocator, block}

#include <rx/core/traits/is_same.h>
#include <rx/core/traits/return_type.h>

namespace rx {

// 32-bit: 12 bytes
// 64-bit: 24 bytes
struct bitset {
  using bit_type = rx_u64;

  static constexpr const bit_type k_bit_one{1};
  static constexpr const rx_size k_word_bits{CHAR_BIT * sizeof(bit_type)};

  bitset() = default;

  // construct bitset with |_size| bits using system allocator
  bitset(rx_size _size);

  // construct bitset with |_size| bits using allocator |_allocator|
  bitset(memory::allocator* _allocator, rx_size _size);

  // move construct from |_bitset|
  bitset(bitset&& _bitset);

  ~bitset();

  // set |_bit|
  void set(rx_size _bit);

  // clear |_bit|
  void clear(rx_size _bit);

  // clear all bits
  void clear_all();

  // test if bit |_bit| is set
  bool test(rx_size _bit) const;

  // the amount of bits
  rx_size size() const;

  // count the # of set bits
  rx_size count_set_bits() const;

  // count the # of unset bits
  rx_size count_unset_bits() const;

  // find the index of the first set bit
  rx_size find_first_set() const;

  // find the index of the first unset bit
  rx_size find_first_unset() const;

  // iterate bitset invoking |_function| with index of each set bit
  template<typename F>
  void each_set(F&& _function);

  // iterate bitset invoking |_function| with index of each unset bit
  template<typename F>
  void each_unset(F&& _function);

private:
  static rx_size index(rx_size bit);
  static rx_size offset(rx_size bit);

  memory::allocator* m_allocator;
  rx_size m_size;
  bit_type* m_data;
};

inline bitset::bitset(bitset&& set)
  : m_allocator{set.m_allocator}
  , m_data{set.m_data}
{
  set.m_data = nullptr;
}

inline bitset::~bitset() {
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(m_data));
}

inline void bitset::set(rx_size bit) {
  RX_ASSERT(bit < m_size, "out of bounds");
  m_data[index(bit)] |= k_bit_one << offset(bit);
}

inline void bitset::clear(rx_size bit) {
  RX_ASSERT(bit < m_size, "out of bounds");
  m_data[index(bit)] &= ~(k_bit_one << offset(bit));
}

inline bool bitset::test(rx_size bit) const {
  RX_ASSERT(bit < m_size, "out of bounds");
  return !!(m_data[index(bit)] & (k_bit_one << offset(bit)));
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

template<typename F>
inline void bitset::each_set(F&& _function) {
  for (rx_size i{0}; i < m_size; i++) {
    if (test(i)) {
      if constexpr (traits::is_same<bool, traits::return_type<F>>) {
        if (!_function(i)) {
          return;
        }
      } else {
        _function(i);
      }
    }
  }
}

template<typename F>
inline void bitset::each_unset(F&& _function) {
  for (rx_size i{0}; i < m_size; i++) {
    if (!test(i)) {
      if constexpr (traits::is_same<bool, traits::return_type<F>>) {
        if (!_function(i)) {
          return;
        }
      } else {
        _function(i);
      }
    }
  }
}

} // namespace rx

#endif // RX_CORE_BITSET_H
