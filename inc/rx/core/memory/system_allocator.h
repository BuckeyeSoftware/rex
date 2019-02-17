#ifndef RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#define RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H

#include <rx/core/memory/allocator.h>

namespace rx::memory {

// system allocator
struct system_allocator final : allocator {
  virtual block allocate(rx_size size);
  virtual block reallocate(block&& data, rx_size);
  virtual void deallocate(block&& data);
  virtual bool owns(const block& data) const;
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
