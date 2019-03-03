#include <stdlib.h> // malloc, realloc, free

#include <rx/core/memory/system_allocator.h> // system_allocator

namespace rx::memory {

block system_allocator::allocate(rx_size size) {
  return {size, reinterpret_cast<rx_byte*>(malloc(size))};
}

block system_allocator::reallocate(block& data, rx_size size) {
  return {size, reinterpret_cast<rx_byte*>(realloc(data ? data.data() : nullptr, size))};
}

void system_allocator::deallocate(block&& data) {
  if (data) {
    free(data.data());
  }
}

bool system_allocator::owns(const block&) const {
  return true;
}

static_global<system_allocator> g_system_allocator("system_allocator");

} // namespace rx::memory
