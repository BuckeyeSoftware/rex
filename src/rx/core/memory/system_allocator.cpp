#include <stdlib.h> // malloc, realloc, free

#include <rx/core/memory/system_allocator.h> // system_allocator
#include <rx/core/concurrency/scope_lock.h>
#include <rx/core/algorithm/max.h>

namespace rx::memory {

struct malloc_allocator : allocator {
  virtual rx_byte* allocate(rx_size _size) {
    return reinterpret_cast<rx_byte*>(malloc(_size));
  }

  virtual rx_byte* reallocate(rx_byte* _data, rx_size _size) {
    return reinterpret_cast<rx_byte*>(realloc(_data, _size));
  }

  virtual void deallocate(rx_byte* _data) {
    free(_data);
  }
};

static_global<malloc_allocator> g_malloc_allocator("malloc_allocator");
static_global<stats_allocator> g_system_allocator("system_allocator", &g_malloc_allocator);

} // namespace rx::memory
