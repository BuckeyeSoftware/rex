#ifndef RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
#define RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
#include "rx/core/memory/allocator.h"

namespace rx::memory {

struct electric_fence_allocator
  final : allocator
{
  electric_fence_allocator() = default;

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(void* _data, rx_size _size);
  virtual void deallocate(void* data);
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
