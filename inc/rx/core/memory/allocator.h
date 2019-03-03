#ifndef RX_CORE_MEMORY_ALLOCATOR_H
#define RX_CORE_MEMORY_ALLOCATOR_H

#include <rx/core/memory/block.h> // block
#include <rx/core/concepts/interface.h> // interface

namespace rx::memory {

struct allocator : concepts::interface {
  static constexpr const rx_size k_alignment = 16;

  constexpr allocator() = default;
  ~allocator() = default;

  // allocate block of size |size|
  virtual block allocate(rx_size size) = 0;

  // reallocate block |data| to size |size|
  virtual block reallocate(block& data, rx_size) = 0;

  // reallocate block |data|
  virtual void deallocate(block&& data) = 0;

  // check if allocator owns block |data|
  virtual bool owns(const block& data) const = 0;

  static constexpr rx_size round_to_alignment(rx_size size);
};

inline constexpr rx_size allocator::round_to_alignment(rx_size size) {
  if (size & (k_alignment - 1)) {
    return (size + k_alignment - 1) & -k_alignment;
  }
  return size;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_ALLOCATOR_H
