#include <string.h> // memcpy

#include "rx/core/memory/copy.h"

namespace Rx::Memory {

void* copy_untyped(void *RX_HINT_RESTRICT dst_, const void *RX_HINT_RESTRICT _src, Size _bytes) {
  // Help check for undefined calls to memcpy.
  RX_ASSERT(dst_, "null destination");
  RX_ASSERT(_src, "null source");

  // TODO(dweiler): implement our own.
  return memcpy(dst_, _src, _bytes);
}

} // namespace Rx::Memory