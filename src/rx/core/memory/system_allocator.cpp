#include "rx/core/memory/system_allocator.h"

namespace rx::memory {

// stats
rx_byte* system_allocator::allocate(rx_size _size) {
#if defined(RX_DEBUG)
  return m_debug_allocator.allocate(_size);
#else
  return m_stats_allocator.allocate(_size);
#endif
}

rx_byte* system_allocator::reallocate(void* _data, rx_size _size) {
#if defined(RX_DEBUG)
  return m_debug_allocator.reallocate(_data, _size);
#else
  return m_stats_allocator.reallocate(_data, _size);
#endif
}

void system_allocator::deallocate(void* _data) {
#if defined(RX_DEBUG)
  return m_debug_allocator.deallocate(_data);
#else
  return m_stats_allocator.deallocate(_data);
#endif
}

global<system_allocator> system_allocator::s_instance{"system", "allocator"};

} // namespace rx::memory
