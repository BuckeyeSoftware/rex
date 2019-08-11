#ifndef RX_CORE_MEMORY_BUMP_POINT_ALLOCATOR_H
#define RX_CORE_MEMORY_BUMP_POINT_ALLOCATOR_H
#include "rx/core/types.h"

namespace rx::memory {

struct allocator;

// linear burn / scratch memory, no reallocations or deallocations, just bumps
// a pointer along based on the allocation size
struct bump_point_allocator {
  bump_point_allocator() = delete;
  bump_point_allocator(allocator* _allocator, rx_size _size);
  ~bump_point_allocator();

  rx_byte* allocate(rx_size _size);
  void reset();

  rx_size size() const;
  rx_size used() const;

private:
  allocator* m_allocator;
  rx_size m_size;
  rx_byte* m_data;
  rx_byte* m_point;
};

inline rx_size bump_point_allocator::used() const {
  return m_point - m_data;
}

inline rx_size bump_point_allocator::size() const {
  return m_size;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_BUMP_POINT_ALLOCATOR_H
