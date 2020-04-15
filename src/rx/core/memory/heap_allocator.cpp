#include <stdlib.h> // malloc, realloc, free

#include "rx/core/memory/heap_allocator.h"

namespace rx::memory {

global<heap_allocator> heap_allocator::s_instance{"system", "heap_allocator"};

rx_byte* heap_allocator::allocate(rx_size _size) {
  return reinterpret_cast<rx_byte*>(malloc(_size));
}

rx_byte* heap_allocator::reallocate(void* _data, rx_size _size) {
  return reinterpret_cast<rx_byte*>(realloc(_data, _size));
}

void heap_allocator::deallocate(void* _data) {
  free(_data);
}

} // namespace rx::memory
