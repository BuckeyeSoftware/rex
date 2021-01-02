#ifndef RX_CORE_UTILITY_MOVE_H
#define RX_CORE_UTILITY_MOVE_H
#include "rx/core/traits/remove_reference.h"

namespace Rx::Utility {

template<typename T>
constexpr traits::remove_reference<T>&& move(T&& _value) {
  return static_cast<traits::remove_reference<T>&&>(_value);
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_MOVE_H
