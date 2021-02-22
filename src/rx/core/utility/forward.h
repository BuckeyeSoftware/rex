#ifndef RX_CORE_UTILITY_FORWARD_H
#define RX_CORE_UTILITY_FORWARD_H
#include "rx/core/traits/remove_reference.h"
#include "rx/core/traits/is_lvalue_reference.h"

#include "rx/core/hints/force_inline.h"

namespace Rx::Utility {

template<typename T>
RX_HINT_FORCE_INLINE constexpr T&& forward(Traits::RemoveReference<T>& _value) {
  return static_cast<T&&>(_value);
}

template<typename T>
RX_HINT_FORCE_INLINE constexpr T&& forward(Traits::RemoveReference<T>&& _value) {
  static_assert(!Traits::IS_LVALUE_REFERENCE<T>,
    "cannot forward an rvalue as an lvalue");
  return static_cast<T&&>(_value);
}

} // namespace utility

#endif // RX_CORE_UTILITY_FORWARD_H
