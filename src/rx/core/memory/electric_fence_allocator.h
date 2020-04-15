#ifndef RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
#define RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
#include "rx/core/concurrency/mutex.h"

#include "rx/core/memory/allocator.h"
#include "rx/core/memory/vma.h"

#include "rx/core/map.h"
#include "rx/core/global.h"

namespace rx::memory {

// # Electric Fence
//
// Special type of allocator which uses VMAs to construct allocations that are
// surrounded by inacessible pages to help detect buffer under- and over- flows.
//
// This uses significant amounts of memory and should only be used for debugging
// memory corruption issues.
struct electric_fence_allocator
  final : allocator
{
  electric_fence_allocator();

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(void* _data, rx_size _size);
  virtual void deallocate(void* data);

  static constexpr allocator& instance();

private:
  vma* allocate_vma(rx_size _size);

  concurrency::spin_lock m_lock;
  map<rx_byte*, vma> m_mappings; // protected by |m_lock|.

  static global<electric_fence_allocator> s_instance;
};

inline constexpr allocator& electric_fence_allocator::instance() {
  return *s_instance;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
