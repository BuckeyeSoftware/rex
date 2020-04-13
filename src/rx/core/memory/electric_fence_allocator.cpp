#include <string.h>
#include <stdio.h> // printf

#include "rx/core/memory/electric_fence_allocator.h"
#include "rx/core/assert.h"
#include "rx/core/hints/likely.h"

#if defined(RX_PLATFORM_POSIX)
#include <sys/mman.h> // mmap, mprotect
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#error "missing eletric fence implementation"
#endif

rx_byte* vm_allocate(rx_size _size) {
#if defined(RX_PLATFORM_POSIX)
  const auto prot = PROT_READ | PROT_WRITE;
  const auto flags = MAP_PRIVATE | MAP_ANONYMOUS;
  if (auto result = mmap(nullptr, _size, prot, flags, -1, 0); result != MAP_FAILED) {
    return reinterpret_cast<rx_byte*>(result);
  }
  return nullptr;
#elif defined(RX_PLATFORM_WINDOWS)
  auto result = VirtualAlloc(nullptr, _size, MEM_COMMIT, PAGE_READWRITE);
  return reinterpret_cast<rx_byte*>(result);
#endif
}

void vm_free(void* _address, rx_size _size) {
#if defined(RX_PLATFORM_POSIX)
  munmap(_address, _size);
#elif defined(RX_PLATFORM_WINDOWS)
  VirtualFree(_address, _size, MEM_DECOMMIT);
#endif
}

void vm_allow_access(void* _address, rx_size _size) {
#if defined(RX_PLATFORM_POSIX)
  RX_ASSERT(mprotect(_address, _size, PROT_READ | PROT_WRITE) == 0,
    "failed to allow access to page range [%p, %p]", _address,
    reinterpret_cast<rx_byte*>(_address) + _size);
#elif defined(RX_PLATFORM_WINDOWS)
  DWORD old;
  RX_ASSERT(VirtualProtect(_address, _size, PAGE_READWRITE, &old),
    "failed to allow access to page range [%p, %p]", _address,
    reinterpret_cast<rx_byte*>(_address) + _size);
#endif
}

void vm_deny_access(void* _address, rx_size _size) {
#if defined(RX_PLATFORM_POSIX)
  RX_ASSERT(mprotect(_address, _size, PROT_NONE) == 0,
    "failed to deny access to page range [%p, %p]", _address,
    reinterpret_cast<rx_byte*>(_address) + _size);
#elif defined(RX_PLATFORM_WINDOWS)
  DWORD old;
  RX_ASSERT(VirtualProtect(_address, _size, PAGE_NOACCESS, &old),
    "failed to deny access to page range [%p, %p]", _address,
    reinterpret_cast<rx_byte*>(_address) + _size);
#endif
}

namespace rx::memory {

static constexpr const rx_size k_page_size = 4096;

static inline rx_size adjust_size(rx_size _size) {
  return ((_size + k_page_size * 2) + k_page_size - 1) & -k_page_size;
}

rx_byte* electric_fence_allocator::allocate(rx_size _size) {
  const auto size = adjust_size(_size);
  if (auto data = vm_allocate(size)) {
    // Write the size to the first page before we make it inaccessible.
    *reinterpret_cast<rx_size*>(data) = size;

    // The first and last page is inaccessible.
    vm_deny_access(data, k_page_size);
    vm_deny_access(data + size - k_page_size, k_page_size);

    // Our allocation begins right after the first page.
    return data + k_page_size;
  }

  return nullptr;
}

rx_byte* electric_fence_allocator::reallocate(void* _data, rx_size _size) {
  if (RX_HINT_LIKELY(_data)) {
    auto data = reinterpret_cast<rx_byte*>(_data) - k_page_size;

    // Read the size by temporarily making the page accessible again.
    vm_allow_access(data, k_page_size);
    const auto old_size = *reinterpret_cast<rx_size*>(data);
    vm_deny_access(data, k_page_size);

    // The allocation does not need to be resized.
    const auto new_size = adjust_size(_size);
    if (old_size >= new_size) {
      return data + k_page_size;
    }

    // Resize the allocation by allocating fresh storage and copying.
    if (auto resize = vm_allocate(new_size)) {
      *reinterpret_cast<rx_size*>(resize) = new_size;

      // The first and last page is inaccessible.
      vm_deny_access(resize, k_page_size);
      vm_deny_access(resize + new_size - k_page_size, k_page_size);

      // Copy the contents.
      memcpy(resize + k_page_size, data + k_page_size, old_size - k_page_size * 2);

      // Free the old mapping.
      vm_free(data, old_size);

      // Our allocation begins after the first page.
      return resize + k_page_size;
    }
  }

  return allocate(_size);
}

void electric_fence_allocator::deallocate(void* _data) {
  if (RX_HINT_LIKELY(_data)) {
    auto data = reinterpret_cast<rx_byte*>(_data) - k_page_size;

    // Make the first page accessible to read the size.
    vm_allow_access(data, k_page_size);

    // Read the size.
    auto size = *reinterpret_cast<rx_size*>(data);

    // Release all the pages.
    vm_free(data, size);
  }
}

} // namespace rx::memory
