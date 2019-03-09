#ifndef RX_CORE_MEMORY_STACK_ALLOCATOR_H
#define RX_CORE_MEMORY_STACK_ALLOCATOR_H

#include <rx/core/memory/allocator.h>

namespace rx::memory {

// linear burn allocator
struct stack_allocator final : allocator {
  stack_allocator(allocator* base, rx_size size);
  ~stack_allocator();

  virtual block allocate(rx_size size);
  virtual block reallocate(block& data, rx_size);
  virtual void deallocate(block&& data);
  virtual bool owns(const block& data) const;
  void reset();

private:
  allocator* m_base;
  block m_data;
  rx_byte* m_point;
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_STACK_ALLOCATOR_H
