#include <string.h> // memcpy

#include "rx/core/memory/electric_fence_allocator.h"
#include "rx/core/memory/heap_allocator.h"

#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/hints/likely.h"
#include "rx/core/hints/unlikely.h"

#include "rx/core/abort.h"
#include "rx/core/vector.h"

namespace rx::memory {

global<electric_fence_allocator> electric_fence_allocator::s_instance{"system", "electric_fence_allocator"};

static constexpr const rx_size k_page_size = 4096;

static inline rx_size pages_needed(rx_size _size) {
  const auto rounded_size = (_size + (k_page_size - 1)) & ~(k_page_size - 1);
  return 2 + rounded_size / k_page_size;
}

electric_fence_allocator::electric_fence_allocator()
  : m_mappings{heap_allocator::instance()}
{
}

vma* electric_fence_allocator::allocate_vma(rx_size _size) {
  const auto pages = pages_needed(_size);

  // Create a new mapping with no permissions.
  vma mapping;
  if (RX_HINT_UNLIKELY(!mapping.allocate(k_page_size, pages))) {
    return nullptr;
  }

  // Commit all pages except the first and last one.
  if (RX_HINT_UNLIKELY(!mapping.commit({1, pages - 2}, true, true))) {
    return nullptr;
  }

  return m_mappings.insert(mapping.base(), utility::move(mapping));
}

rx_byte* electric_fence_allocator::allocate(rx_size _size) {
  concurrency::scope_lock lock{m_lock};
  if (auto mapping = allocate_vma(_size)) {
    return mapping->page(1);
  }
  return nullptr;
}

rx_byte* electric_fence_allocator::reallocate(void* _data, rx_size _size) {
  if (RX_HINT_LIKELY(_data)) {
    concurrency::scope_lock lock{m_lock};
    const auto base = reinterpret_cast<rx_byte*>(_data) - k_page_size;
    if (auto mapping = m_mappings.find(base)) {
      // No need to reallocate, allocation still fits inside.
      if (mapping->page_count() >= pages_needed(_size)) {
        return mapping->page(1);
      }

      if (auto resize = allocate_vma(_size)) {
        // Copy all pages except the electric fence pages on either end.
        const auto size = mapping->page_size() * (mapping->page_count() - 2);
        memcpy(resize->page(1), mapping->page(1), size);

        // Release the smaller VMA.
        m_mappings.erase(base);

        return resize->page(1);
      }

      return nullptr;
    }

    abort("invalid reallocate");
  }

  return allocate(_size);
}

void electric_fence_allocator::deallocate(void* _data) {
  if (RX_HINT_LIKELY(_data)) {
    concurrency::scope_lock lock{m_lock};
    const auto base = reinterpret_cast<rx_byte*>(_data) - k_page_size;
    if (!m_mappings.erase(base)) {
      abort("invalid deallocate");
    }
  }
}

} // namespace rx::memory
