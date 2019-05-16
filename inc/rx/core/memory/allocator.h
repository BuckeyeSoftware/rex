#ifndef RX_CORE_MEMORY_ALLOCATOR_H
#define RX_CORE_MEMORY_ALLOCATOR_H

#include <rx/core/types.h> // rx_size, rx_byte
#include <rx/core/concepts/interface.h> // concepts::interface

namespace rx::memory {

struct allocator : concepts::interface {
  // all allocators must align their data and round their sizes to this alignment
  // value as well, failure to do so will lead to unaligned reads and writes to
  // several engine interfaces that depend on this behavior and possible crashes
  // in interfaces that rely on alignment for SIMD and being able to tag pointer
  // bits with additional information.
  //
  // rounding of pointers and sizes can be done with round_to_alignment
  static constexpr const rx_size k_alignment = 16;

  constexpr allocator() = default;
  ~allocator() = default;

  // allocate memory of size |_size|
  virtual rx_byte* allocate(rx_size _size) = 0;

  // reallocate existing memory |_data| to size |_size|, should be an alias for
  // allocate(_size) when |_data| is nullptr
  virtual rx_byte* reallocate(rx_byte* _data, rx_size _size) = 0;

  // reallocate existing memory |_data|
  virtual void deallocate(rx_byte* data) = 0;

  static constexpr rx_uintptr round_to_alignment(rx_uintptr _ptr_or_size);
};

inline constexpr rx_uintptr allocator::round_to_alignment(rx_uintptr _ptr_or_size) {
  return (_ptr_or_size + (k_alignment - 1)) & ~(k_alignment - 1);
}

struct view {
  allocator* owner;
  rx_byte* data;
  rx_size size;
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_ALLOCATOR_H
