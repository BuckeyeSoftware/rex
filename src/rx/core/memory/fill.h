#ifndef RX_CORE_MEMORY_FILL_H
#define RX_CORE_MEMORY_FILL_H
#include "rx/core/types.h"

namespace Rx::Memory {

void* fill_untyped(void* dest_, Byte _value, Size _size);

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_FILL_H