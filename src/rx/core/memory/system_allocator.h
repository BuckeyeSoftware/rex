#ifndef RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#define RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#include "rx/core/memory/heap_allocator.h"
#include "rx/core/memory/stats_allocator.h"

#include "rx/core/global.h"

namespace rx::memory {

// # System Allocator
//
// The generalized system allocator. Built off a heap allocator and a stats
// allocator to track global system allocations. When something isn't provided
// an allocator, this is the allocator used. More specifically, the global
// g_system_allocator is used.
struct system_allocator
  final : allocator
{
  constexpr system_allocator();

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(void* _data, rx_size _size);
  virtual void deallocate(void* _data);

  stats_allocator::statistics stats() const;

  static constexpr allocator& instance();

private:
  heap_allocator m_heap_allocator;
  stats_allocator m_stats_allocator;

  static global<system_allocator> s_instance;
};

inline constexpr system_allocator::system_allocator()
  : m_stats_allocator{m_heap_allocator}
{
}

inline stats_allocator::statistics system_allocator::stats() const {
  return m_stats_allocator.stats();
}

inline constexpr allocator& system_allocator::instance() {
  return *s_instance;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
