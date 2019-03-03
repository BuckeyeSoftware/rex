#ifndef RX_CORE_MEMORY_POOL_ALLOCATOR_H
#define RX_CORE_MEMORY_POOL_ALLOCATOR_H

#include <rx/core/bitset.h> // bitset
#include <rx/core/concepts/no_copy.h> // no_copy
#include <rx/core/concepts/no_move.h> // no_move

namespace rx::memory {

struct allocator;

struct pool_allocator
  : concepts::no_copy
  , concepts::no_move
{
  pool_allocator(allocator* alloc, rx_size size, rx_size count);
  ~pool_allocator();

  block allocate();
  void deallocate(block&& data);
  bool owns(const block& data) const;
  rx_size index_of(const block& data) const;
  block data_of(rx_size index) const;
  rx_size size() const;

private:
  allocator* m_allocator;
  rx_size m_size;
  block m_data;
  bitset m_bits;
};

inline bool pool_allocator::owns(const block& data) const {
  return data.data() >= m_data.data() && data.end() <= m_data.end();
}

inline rx_size pool_allocator::index_of(const block& data) const {
  RX_ASSERT(owns(data), "pool does not own block");
  return (data.cast<rx_size>() - m_data.cast<rx_size>()) / m_size;
}

inline block pool_allocator::data_of(rx_size index) const {
  return {m_size, m_data.data() + index * m_size};
}

inline rx_size pool_allocator::size() const {
  return m_size;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_POOL_ALLOCATOR_H

