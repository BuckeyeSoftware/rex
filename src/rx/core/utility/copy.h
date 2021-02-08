#ifndef RX_CORE_UTILITY_COPY_H
#define RX_CORE_UTILITY_COPY_H
#include "rx/core/optional.h"

#include "rx/core/traits/is_same.h"

namespace Rx::Utility {

template<typename T1, typename T2>
concept Returns = Traits::IS_SAME<T1, T2>;

template<typename T>
concept Copyable = requires(const T& _value) {
  { T::copy(_value) } -> Returns<Optional<T>>;
};

template<typename T>
Optional<T> copy(const T& _value) {
  if constexpr (Copyable<T>) {
    return T::copy(_value);
  } else {
    return _value;
  }
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_COPY_H