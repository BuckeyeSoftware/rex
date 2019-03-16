#ifndef RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#define RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H

#include <rx/core/memory/allocator.h> // allocator
#include <rx/core/statics.h> // static_global

namespace rx::memory {

// system allocator
struct system_allocator final : allocator {
  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(rx_byte* _data, rx_size);
  virtual void deallocate(rx_byte* _data);
  virtual bool owns(const rx_byte* _data) const;
};

extern static_global<system_allocator> g_system_allocator;

} // namespace rx::memory

#endif // RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
