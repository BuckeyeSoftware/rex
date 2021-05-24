#ifndef RX_CORE_MEMORY_ZERO_H
#define RX_CORE_MEMORY_ZERO_H
#include "rx/core/assert.h"
#include "rx/core/concepts/trivial.h"
#include "rx/core/memory/fill.h"

namespace Rx::Memory {

inline void zero_untyped(void* dst_, Size _size) {
  fill_untyped(dst_, 0, _size);
}

template<Concepts::Trivial T>
inline void zero(T& object_) {
  zero_untyped(&object_, sizeof(T));
}

template<Concepts::Trivial T, Size E>
inline void zero(T (&object_)[E]) {
  zero_untyped(&object_, sizeof(T[E]));
}

template<Concepts::Trivial T>
inline void zero(T* dest_, Size _elements) {
  // Check for overflow of sizeof(T) * _elements, when sizeof(T) != 1.
  if constexpr(sizeof(T) != 1) {
    RX_ASSERT(_elements <= -1_z / sizeof(T), "overflows");
  }
  zero_untyped(dest_, sizeof(T) * _elements);
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_ZERO_H