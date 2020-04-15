#ifndef RX_CORE_MEMORY_HEAP_ALLOCATOR_H
#define RX_CORE_MEMORY_HEAP_ALLOCATOR_H
#include "rx/core/memory/allocator.h"

#include "rx/core/global.h"

namespace rx::memory {

// # Heap Allocator
//
// The generalized heap allocator. This is how memory is allocated from the
// operating system and returned to it. On most systems this just wraps the C
// standard functions: malloc, realloc and free.
//
// The purpose of this allocator is to provide a generic, heap allocator
// that can be used for anything.
struct heap_allocator
  final : allocator
{
  constexpr heap_allocator() = default;

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(void* _data, rx_size _size);
  virtual void deallocate(void* _data);

  static constexpr allocator& instance();

private:
  static global<heap_allocator> s_instance;
};

inline constexpr allocator& heap_allocator::instance() {
  return *s_instance;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_HEAP_ALLOCATOR_H
