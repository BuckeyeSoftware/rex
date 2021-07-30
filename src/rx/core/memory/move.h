#ifndef RX_CORE_MEMORY_MOVE_H
#define RX_CORE_MEMORY_MOVE_H
#include "rx/core/assert.h"
#include "rx/core/concepts/trivially_copyable.h"
#include "rx/core/hints/restrict.h"

namespace Rx::Memory {

RX_API void move_untyped(void* dst_, const void* _src, Size _bytes);

template<Concepts::TriviallyCopyable T>
void move(T* dst_, const T* _src, Size _elements) {
  // Check for sizeof(T) * _elements overflow.
  if constexpr (sizeof(T) != 1) {
    RX_ASSERT(_elements <= -1_z / sizeof(T), "overflow");
  }

  move_untyped(dst_, _src, sizeof(T) * _elements);
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_COPY_H