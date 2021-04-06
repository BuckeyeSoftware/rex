#ifndef RX_CORE_MEMORY_BUDDY_ALLOCATOR_H
#define RX_CORE_MEMORY_BUDDY_ALLOCATOR_H
#include "rx/core/memory/allocator.h"
#include "rx/core/concurrency/spin_lock.h"

/// \file buddy_allocator.h
namespace Rx::Memory {

/// \brief Buddy memory allocator
///
/// Implements the buddy memory allocation algorithm as described by
/// https://en.wikipedia.org/wiki/Buddy_memory_allocation
struct RX_API BuddyAllocator
  final : Allocator
{
  BuddyAllocator(Byte* _data, Size _size);

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void* _data, Size _size);
  virtual void deallocate(void* _data);

private:
  Byte* allocate_unlocked(Size _size);
  Byte* reallocate_unlocked(void* _data, Size _size);
  void deallocate_unlocked(void* _data);

  Concurrency::SpinLock m_lock;

  void* m_head RX_HINT_GUARDED_BY(m_lock);
  void* m_tail RX_HINT_GUARDED_BY(m_lock);
};

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_BUDDY_ALLOCATOR_H
