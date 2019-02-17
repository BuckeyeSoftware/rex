#ifndef RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#define RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H

#include <rx/core/memory/allocator.h>

#include <rx/core/statics.h> // static_global

namespace rx::memory {

// system allocator
struct system_allocator final : allocator {
  virtual block allocate(rx_size size);
  virtual block reallocate(block&& data, rx_size);
  virtual void deallocate(block&& data);
  virtual bool owns(const block& data) const;
};

extern static_global<system_allocator> g_system_allocator;

} // namespace rx::memory

#endif // RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
