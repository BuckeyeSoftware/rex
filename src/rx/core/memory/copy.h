#ifndef RX_CORE_MEMORY_COPY_H
#define RX_CORE_MEMORY_COPY_H
#include "rx/core/assert.h"
#include "rx/core/concepts/trivially_copyable.h"
#include "rx/core/hints/restrict.h"

namespace Rx::Memory {

void copy_untyped(void *RX_HINT_RESTRICT dst_, const void *RX_HINT_RESTRICT _src, Size _bytes);

template<Concepts::TriviallyCopyable T>
void copy(T *RX_HINT_RESTRICT dst_, const T *RX_HINT_RESTRICT _src, Size _elements = 1) {
  // Check for sizeof(T) * _elements overflow.
  if constexpr (sizeof(T) != 1) {
    RX_ASSERT(_elements <= -1_z / sizeof(T), "overflow");
  }

  copy_untyped(dst_, _src, sizeof(T) * _elements);
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_COPY_H