#include "rx/core/memory/vma.h"
#include "rx/core/concurrency/scope_lock.h"

#if defined(RX_PLATFORM_POSIX)
#include <sys/mman.h> // mmap, munmap, mprotect, posix_madvise, MAP_{FAILED,HUGETLB}, PROT_{NONE,READ,WRITE}, POSIX_MADV_{WILLNEED,DONTNEED}
#include <stdlib.h> // mkstemp
#include <unistd.h> // ftruncate
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // VirtualAlloc, VirtualFree, MEM_{RELEASE,COMMIT,UNCOMMIT}, PAGE_{READWRITE,READONLY}
#else
#error "missing VMA implementation"
#endif

namespace Rx::Memory {

void VMA::deallocate() {
  const auto size = m_page_size * m_page_count;

#if defined(RX_PLATFORM_POSIX)
  if (m_shared != -1) {
    RX_ASSERT(close(m_shared) == 0, "close failed");
  }

  if (m_base) {
    RX_ASSERT(munmap(m_base, size) == 0, "munmap failed");
  }
#elif defined(RX_PLATFORM_WINDOWS)
  if (m_base) {
    RX_ASSERT(VirtualFree(m_base, 0, MEM_RELEASE), "VirtualFree failed");
  }
#endif
}

Optional<VMA> VMA::allocate(Size _page_size, Size _page_count, bool _remappable) {
  const auto size = _page_size * _page_count;
#if defined(RX_PLATFORM_POSIX)
  int shared = -1;
  if (_remappable) {
    char path[] = "/tmp/rx-mem-XXXXXX";
    if ((shared = mkstemp(path)) < 0) {
      return nullopt;
    }

    if (unlink(path) || ftruncate(shared, size)) {
      close(shared);
      return nullopt;
    }
  }

  const auto flags = MAP_PRIVATE | MAP_ANONYMOUS;
  const auto map = mmap(nullptr, size, PROT_NONE, flags, -1, 0);
  if (map == MAP_FAILED) {
    return nullopt;
  }

  // Ensure these pages are not comitted initially.
  if (posix_madvise(map, size, POSIX_MADV_DONTNEED) != 0) {
      munmap(map, size);
      return nullopt;
  }

  return VMA {
    reinterpret_cast<Byte*>(map),
    shared,
    _page_size,
    _page_count,
  };
#elif defined(RX_PLATFORM_WINDOWS)
  const auto size = _page_size * _page_count;
  const auto map = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
  if (!map) {
    return nullopt;
  }

  return VMA {
    reinterpret_cast<Byte*>(map),
    -1,
    _page_size,
    _page_count
  };
#endif
  return nullopt;
}

Optional<VMA> VMA::remap(Range _range, bool _read, bool _write) {
  if (m_shared < 0 || !in_range(_range)) {
    return nullopt;
  }

#if defined(RX_PLATFORM_POSIX)
  const auto size = m_page_size * _range.count;
  const auto prot = (_read ? PROT_READ : 0) | (_write ? PROT_WRITE : 0);
  const auto flags = MAP_FIXED | MAP_SHARED;
  const auto map = mmap(m_base + _range.offset * m_page_size, size, prot, flags, m_shared, 0);
  if (map == MAP_FAILED) {
    return nullopt;
  }

  return VMA {
    reinterpret_cast<Byte*>(map),
    -1,
    m_page_size,
    _range.count,
  };
#elif defined(RX_PLATFORM_WINDOWS)
  // TODO(dweiler): Implement...
#endif
  return nullopt;
}

bool VMA::commit(Range _range, bool _read, bool _write) {
  // Cannot commit memory unless one of |_read| or |_write| is true.
  if (!_read && !_write) {
    return false;

  }

  // Cannot commit memory unless in range of the VMA.
  if (!in_range(_range)) {
    return false;
  }

  const auto size = m_page_size * _range.count;
  const auto addr = m_base + m_page_size * _range.offset;

#if defined(RX_PLATFORM_POSIX)
  const auto prot = (_read ? PROT_READ : 0) | (_write ? PROT_WRITE : 0);
  // Ensure the mapping has the correct permissions.
  if (mprotect(addr, size, prot) == 0) {
    // Commit the memory.
    return posix_madvise(addr, size, POSIX_MADV_WILLNEED) == 0;
  }
#elif defined(RX_PLATFORM_WINDOWS)
  const DWORD protect = _write ? PAGE_READWRITE : PAGE_READONLY;
  // Commit the memory.
  return VirtualAlloc(addr, size, MEM_COMMIT, protect);
#endif
  return false;
}

bool VMA::uncommit(Range _range) {
  // Cannot commit memory unless in range of the VMA.
  if (!in_range(_range)) {
    return false;
  }

  const auto size = m_page_size * _range.count;
  const auto addr = m_base + m_page_size * _range.offset;
#if defined(RX_PLATFORM_POSIX)
  return posix_madvise(addr, size, POSIX_MADV_DONTNEED) == 0;
#elif defined(RX_PLATFORM_WINDOWS)
  return VirtualFree(addr, size, MEM_DECOMMIT);
#endif
  return false;
}

} // namespace Rx::Memory
