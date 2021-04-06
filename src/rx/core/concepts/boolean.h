#ifndef RX_CORE_CONCEPTS_BOOLEAN_H
#define RX_CORE_CONCEPTS_BOOLEAN_H
#include "rx/core/traits/is_same.h"

namespace Rx::Concepts {

template<typename T>
concept Boolean = !Traits::IS_SAME<T, bool>;

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_BOOLEAN_H