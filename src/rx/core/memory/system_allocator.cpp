#include <stdlib.h> // malloc, realloc, free

#include <rx/core/memory/system_allocator.h> // system_allocator
#include <rx/core/concurrency/scope_lock.h>
#include <rx/core/algorithm/max.h>

namespace rx::memory {

struct header {
  // requested allocation size, the actual size is round_to_alignment(size) + sizeof(header) + k_alignment
  rx_size size;
  rx_byte* base;
};

rx_byte* system_allocator::allocate(rx_size _size) {
  const rx_uintptr size_as_multiple{round_to_alignment(_size)};
  const rx_uintptr actual_size{size_as_multiple + sizeof(header) + k_alignment};
  rx_byte* base{reinterpret_cast<rx_byte*>(malloc(actual_size))};
  rx_byte* aligned{reinterpret_cast<rx_byte*>(round_to_alignment(reinterpret_cast<rx_uintptr>(base) + sizeof(header)))};
  header* node{reinterpret_cast<header*>(aligned) - 1};
  node->size = _size;
  node->base = base;

  {
    concurrency::scope_lock lock(m_lock);
    m_statistics.allocations++;
    m_statistics.used_request_bytes += _size;
    m_statistics.used_actual_bytes += actual_size;
    m_statistics.peak_request_bytes = algorithm::max(m_statistics.peak_request_bytes, m_statistics.used_request_bytes);
    m_statistics.peak_actual_bytes = algorithm::max(m_statistics.peak_actual_bytes, m_statistics.used_actual_bytes);
  }
  return aligned;
}

rx_byte* system_allocator::reallocate(rx_byte* _data, rx_size _size) {
  if (_data) {
    const rx_uintptr size_as_multiple{round_to_alignment(_size)};
    const rx_uintptr actual_size{size_as_multiple + sizeof(header) + k_alignment};
    header* node{reinterpret_cast<header*>(_data) - 1};
    rx_byte* original{node->base};
    const rx_size original_request_size{node->size};
    const rx_size original_actual_size{round_to_alignment(node->size) + sizeof(header) + k_alignment};
    rx_byte* resize{reinterpret_cast<rx_byte*>(realloc(original, actual_size))};
    rx_byte* aligned{reinterpret_cast<rx_byte*>(round_to_alignment(reinterpret_cast<rx_uintptr>(resize) + sizeof(header)))};
    node = reinterpret_cast<header*>(aligned) - 1;
    node->size = _size;
    node->base = resize;

    {
      concurrency::scope_lock lock(m_lock);
      m_statistics.request_reallocations++;
      if (resize == original) {
        m_statistics.actual_reallocations++;
      }
      m_statistics.used_request_bytes -= original_request_size;
      m_statistics.used_actual_bytes -= original_actual_size;
      m_statistics.used_request_bytes += _size;
      m_statistics.used_actual_bytes += actual_size;
    }
    return aligned;
  }

  return allocate(_size);
}

void system_allocator::deallocate(rx_byte* _data) {
  if (_data) {
    header* node{reinterpret_cast<header*>(_data) - 1};
    const rx_size request_size{node->size};
    const rx_size actual_size{round_to_alignment(node->size) + sizeof(header) + k_alignment};

    {
      concurrency::scope_lock lock(m_lock);
      m_statistics.deallocations++;
      m_statistics.used_request_bytes -= request_size;
      m_statistics.used_actual_bytes -= actual_size;
    }
    free(node);
  }
}

bool system_allocator::owns(const rx_byte*) const {
  return true;
}

statistics system_allocator::stats() {
  concurrency::scope_lock lock(m_lock);
  return m_statistics;
}

static_global<system_allocator> g_system_allocator("system_allocator");

} // namespace rx::memory
