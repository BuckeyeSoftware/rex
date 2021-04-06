#include "rx/core/memory/null_allocator.h"
#include "rx/core/abort.h"

namespace Rx::Memory {

Byte* NullAllocator::Instance::allocate(Size) {
  abort("NullAllocator called");
}

Byte* NullAllocator::Instance::reallocate(void*, Size) {
  abort("NullAllocator called");
}

void NullAllocator::Instance::deallocate(void* _data) {
  // deallocate(nullptr) is a no-op and should be allowed by this allocator.
  if (_data) {
    abort("NullAllocator called");
  }
}

} // namespace Rx::Memory