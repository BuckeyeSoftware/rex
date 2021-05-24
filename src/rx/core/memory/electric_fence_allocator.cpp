#include "rx/core/memory/electric_fence_allocator.h"
#include "rx/core/memory/heap_allocator.h"
#include "rx/core/memory/copy.h"

#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/hints/likely.h"
#include "rx/core/hints/unlikely.h"

#include "rx/core/abort.h"
#include "rx/core/vector.h"

namespace Rx::Memory {

Global<ElectricFenceAllocator> ElectricFenceAllocator::s_instance{"system", "electric_fence_allocator"};

static constexpr const Size PAGE_SIZE = 4096;

static inline Size pages_needed(Size _size) {
  const auto rounded_size = (_size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
  return 2 + rounded_size / PAGE_SIZE;
}

ElectricFenceAllocator::ElectricFenceAllocator()
  : m_mappings{HeapAllocator::instance()}
{
}

VMA* ElectricFenceAllocator::allocate_vma(Size _size) {
  const auto pages = pages_needed(_size);

  // Create a new mapping with no permissions.
  VMA mapping;
  if (RX_HINT_UNLIKELY(!mapping.allocate(PAGE_SIZE, pages))) {
    return nullptr;
  }

  // Commit all pages except the first and last one.
  if (RX_HINT_UNLIKELY(!mapping.commit({1, pages - 2}, true, true))) {
    return nullptr;
  }

  return m_mappings.insert(mapping.base(), Utility::move(mapping));
}

Byte* ElectricFenceAllocator::allocate(Size _size) {
  Concurrency::ScopeLock lock{m_lock};
  if (auto mapping = allocate_vma(_size)) {
    return mapping->page(1);
  }
  return nullptr;
}

Byte* ElectricFenceAllocator::reallocate(void* _data, Size _size) {
  if (RX_HINT_LIKELY(_data)) {
    Concurrency::ScopeLock lock{m_lock};
    const auto base = reinterpret_cast<Byte*>(_data) - PAGE_SIZE;
    if (auto mapping = m_mappings.find(base)) {
      // No need to reallocate, allocation still fits inside.
      if (mapping->page_count() >= pages_needed(_size)) {
        return mapping->page(1);
      }

      if (auto resize = allocate_vma(_size)) {
        // Copy all pages except the electric fence pages on either end.
        const auto size = mapping->page_size() * (mapping->page_count() - 2);
        Memory::copy(resize->page(1), mapping->page(1), size);

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

void ElectricFenceAllocator::deallocate(void* _data) {
  if (RX_HINT_LIKELY(_data)) {
    Concurrency::ScopeLock lock{m_lock};
    const auto base = reinterpret_cast<Byte*>(_data) - PAGE_SIZE;
    if (!m_mappings.erase(base)) {
      abort("invalid deallocate");
    }
  }
}

} // namespace Rx::Memory
