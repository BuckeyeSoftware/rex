#include <string.h> // memmove

#include "rx/core/memory/copy.h"

namespace Rx::Memory {

void move_untyped(void* dst_, const void* _src, Size _bytes) {
  // Help check for undefined calls to memcpy.
  RX_ASSERT(dst_, "null destination");
  RX_ASSERT(_src, "null source");

  // TODO(dweiler): implement our own.
  memmove(dst_, _src, _bytes);
}

} // namespace Rx::Memory