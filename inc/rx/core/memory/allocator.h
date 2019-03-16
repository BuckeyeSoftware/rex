#ifndef RX_CORE_MEMORY_ALLOCATOR_H
#define RX_CORE_MEMORY_ALLOCATOR_H

#include <rx/core/types.h> // rx_size, rx_byte
#include <rx/core/concepts/interface.h> // concepts::interface

namespace rx::memory {

struct allocator : concepts::interface {
  // all allocators must align their memory to this alignment
  static constexpr const rx_size k_alignment = 16;

  constexpr allocator() = default;
  ~allocator() = default;

  virtual rx_byte* allocate(rx_size _size) = 0;
  virtual rx_byte* reallocate(rx_byte* _data, rx_size _size) = 0;
  virtual void deallocate(rx_byte* data) = 0;
  virtual bool owns(const rx_byte* _data) const = 0;

  static constexpr rx_size round_to_alignment(rx_size _size);
};

inline constexpr rx_size allocator::round_to_alignment(rx_size _size) {
  if (_size & (k_alignment - 1)) {
    return (_size + k_alignment - 1) & -k_alignment;
  }
  return _size;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_ALLOCATOR_H
