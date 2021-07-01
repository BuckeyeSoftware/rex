#ifndef RX_CORE_UTILITY_COPY_H
#define RX_CORE_UTILITY_COPY_H
#include "rx/core/optional.h"

namespace Rx::Utility {

template<typename T>
Optional<Traits::RemoveReference<T>> copy(const T& _value) {
  if constexpr (Concepts::Copyable<T>) {
    return T::copy(_value);
  } else {
    return _value;
  }
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_COPY_H