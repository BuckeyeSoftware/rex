#ifndef RX_CORE_MEMORY_DEBUG_ALLOCATOR_H
#define RX_CORE_MEMORY_DEBUG_ALLOCATOR_H
#include "rx/core/memory/allocator.h"
#include "rx/core/concurrency/spin_lock.h"

namespace rx::memory {

// # Debug allocator
//
// The debug allocator wraps an existing allocator to give it functionality for
// debugging heap corruption issues.
//
// The core idea here is to stick a canary value before and after all allocated
// memory. During free, these canaries can be checked to see if they've been
// overwritten, which would imply a buffer overflow or underflow.
struct debug_allocator
  final : allocator
{
  // 32-bit: 15 MiB array
  // 64-bit: 30 MiB array
  static inline constexpr const auto k_max_tracking = 3932160;

  constexpr debug_allocator() = delete;
  constexpr debug_allocator(allocator& _allocator);

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(void* _data, rx_size _size);
  virtual void deallocate(void* data);

private:
  rx_byte* track(rx_byte* _data);
  void untrack(rx_byte* _data, const char* _caller);

  allocator& m_allocator;
  concurrency::spin_lock m_lock;
  rx_uintptr m_tracked[k_max_tracking]; // protected by |m_lock|.
};

inline constexpr debug_allocator::debug_allocator(allocator& _allocator)
  : m_allocator{_allocator}
  , m_tracked{0}
{
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_DEBUG_ALLOCATOR_H
