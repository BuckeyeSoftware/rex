#ifndef RX_CORE_CONCEPTS_FLOATING_POINT_H
#define RX_CORE_CONCEPTS_FLOATING_POINT_H
#include "rx/core/traits/is_same.h"

namespace Rx::Concepts {

template<typename T>
concept FloatingPoint =
  Traits::IS_SAME<T, Float32> || Traits::IS_SAME<T, Float64>;

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_FLOATING_POINT_H
