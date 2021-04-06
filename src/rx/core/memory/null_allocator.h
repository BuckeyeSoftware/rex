#ifndef RX_CORE_MEMORY_NULL_ALLOCATOR_H
#define RX_CORE_MEMORY_NULL_ALLOCATOR_H
#include "rx/core/memory/allocator.h"

namespace Rx::Memory {

// Only allow one instance of this.
struct NullAllocator {
  struct Instance : Allocator {
    virtual Byte* allocate(Size);
    virtual Byte* reallocate(void*, Size);
    virtual void deallocate(void*);
  private:
    friend struct NullAllocator;
    constexpr Instance() = default;
  };

  static constexpr Instance& instance();

private:
  static inline Instance s_instance;
};

inline constexpr NullAllocator::Instance& NullAllocator::instance() {
  return s_instance;
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_NULL_ALLOCATOR_H