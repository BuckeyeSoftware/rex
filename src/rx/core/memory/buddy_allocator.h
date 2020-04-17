#ifndef RX_CORE_MEMORY_BUDDY_ALLOCATOR_H
#define RX_CORE_MEMORY_BUDDY_ALLOCATOR_H
#include "rx/core/memory/allocator.h"
#include "rx/core/concurrency/spin_lock.h"

namespace rx::memory {

struct buddy_allocator
  final : allocator
{
  buddy_allocator(rx_byte* _data, rx_size _size);

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(void* _data, rx_size _size);
  virtual void deallocate(void* _data);

private:
  rx_byte* allocate_unlocked(rx_size _size);
  rx_byte* reallocate_unlocked(void* _data, rx_size _size);
  void deallocate_unlocked(void* _data);

  concurrency::spin_lock m_lock;

  void* m_head; // protected by |m_lock|
  void* m_tail; // protected by |m_lock|
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_BUDDY_ALLOCATOR_H
