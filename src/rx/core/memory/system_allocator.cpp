#include <stdlib.h> // malloc, realloc, free

#include <rx/core/memory/system_allocator.h> // system_allocator

namespace rx::memory {

rx_byte* system_allocator::allocate(rx_size _size) {
  return reinterpret_cast<rx_byte*>(malloc(_size));
}

rx_byte* system_allocator::reallocate(rx_byte* _data, rx_size _size) {
  return reinterpret_cast<rx_byte*>(realloc(_data, _size));
}

void system_allocator::deallocate(rx_byte* _data) {
  free(_data);
}

bool system_allocator::owns(const rx_byte*) const {
  return true;
}

static_global<system_allocator> g_system_allocator("system_allocator");

} // namespace rx::memory
