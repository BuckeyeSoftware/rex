#ifndef RX_CORE_CONCEPTS_POINTER_H
#define RX_CORE_CONCEPTS_POINTER_H
#include "rx/core/traits/is_pointer.h"

namespace Rx::Concepts {

template<typename T>
concept Pointer = Traits::IS_POINTER<T>;

} // namespace Rx

#endif // RX_CORE_CONCEPTS_POINTER_H