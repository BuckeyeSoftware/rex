#ifndef RX_CORE_CONCEPTS_ASSIGNABLE_H
#define RX_CORE_CONCEPTS_ASSIGNABLE_H
#include "rx/core/utility/declval.h"

namespace Rx::Concepts {

template<typename T, typename U>
concept Assignable = requires {
  Utility::declval<T>() = Utility::declval<U>();
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_ASSIGNABLE_H
