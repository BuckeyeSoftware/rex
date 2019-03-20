#ifndef RX_CORE_MEMORY_STACK_ALLOCATOR_H
#define RX_CORE_MEMORY_STACK_ALLOCATOR_H

#include <rx/core/memory/allocator.h>

namespace rx::memory {

// linear burn allocator
struct stack_allocator final : allocator {
  stack_allocator() = default;
  stack_allocator(allocator* _allocator, rx_size _size);
  ~stack_allocator();

  virtual rx_byte* allocate(rx_size _size);
  virtual rx_byte* reallocate(rx_byte* _data, rx_size _size);
  virtual void deallocate(rx_byte* _data);
  virtual bool owns(const rx_byte* _data) const;

  void reset();

private:
  allocator* m_allocator;
  rx_byte* m_data;
  rx_size m_size;
  rx_byte* m_point;
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_STACK_ALLOCATOR_H
