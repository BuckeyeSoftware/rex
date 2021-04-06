#ifndef RX_CORE_CONCEPTS_ASSIGNABLE_H
#define RX_CORE_CONCEPTS_ASSIGNABLE_H
#include "rx/core/utility/declval.h"

/// \file assignable.h
namespace Rx::Concepts {

/// Checks if an object of type \p U is assignable to a type \p T.
template<typename T, typename U>
concept Assignable = requires {
  Utility::declval<T>() = Utility::declval<U>();
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_ASSIGNABLE_H
