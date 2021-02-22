#ifndef RX_CORE_UTILITY_MOVE_H
#define RX_CORE_UTILITY_MOVE_H
#include "rx/core/traits/remove_reference.h"
#include "rx/core/hints/force_inline.h"

namespace Rx::Utility {

template<typename T>
RX_HINT_FORCE_INLINE constexpr Traits::RemoveReference<T>&& move(T&& _value) {
  return static_cast<Traits::RemoveReference<T>&&>(_value);
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_MOVE_H
