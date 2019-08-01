#ifndef RX_CORE_MEMORY_STATS_ALLOCATOR_H
#define RX_CORE_MEMORY_STATS_ALLOCATOR_H

#include "rx/core/memory/allocator.h"
#include "rx/core/concurrency/spin_lock.h"

namespace rx::memory {

struct statistics {
  rx_size allocations;           // # of calls to allocate
  rx_size request_reallocations; // # of calls to reallocate
  rx_size actual_reallocations;  // # of calls to reallocate that actually just resized memory in place
  rx_size deallocations;         // # of calls to deallocate

  // measures peak and in-use requested bytes, that is the literal sizes given
  // to allocate and reallocate and not the actual bytes
  rx_u64 peak_request_bytes;
  rx_u64 used_request_bytes;

  // measures peak and in-use actual bytes, these are the actual values after
  // sizes to allocate and reallocate are rounded aligned and tracked with metadata
  rx_u64 peak_actual_bytes;
  rx_u64 used_actual_bytes;
};

// extends an existing allocator with a header of metadata used to track allocations;
// useful for finding memory leaks, reasoning about memory usage of engine systems
// and debugging memory corruption
struct stats_allocator
  : allocator
{
  stats_allocator() = delete;
  stats_allocator(allocator* _allocator);
  ~stats_allocator();

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(rx_byte* _data, rx_size _size);
  virtual void deallocate(rx_byte* data);
  statistics stats();

private:
  allocator* m_allocator;
  concurrency::spin_lock m_lock;
  statistics m_statistics; // protected by |m_lock|
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_STATS_ALLOCATOR_H