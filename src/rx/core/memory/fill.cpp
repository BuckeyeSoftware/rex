#include <string.h> // memset

#include "rx/core/memory/fill.h"

namespace Rx::Memory {

void fill_untyped(void* dest_, Byte _value, Size _size) {
  // TODO(dweiler): implement our own.
  memset(dest_, _value, _size);
}

} // namespace Rx::Memory