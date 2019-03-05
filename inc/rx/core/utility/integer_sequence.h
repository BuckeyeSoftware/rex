#ifndef RX_CORE_UTILITY_INTEGER_SEQUENCE_H
#define RX_CORE_UTILITY_INTEGER_SEQUENCE_H

#include <rx/core/traits/type_identity.h>

namespace rx::utility {

template<typename T, T... Is>
struct integer_sequence
  : traits::type_identity<integer_sequence<T, Is...>>
{
};

} // namespace rx::utility

#endif // RX_CORE_UTILITY_INTEGER_SEQUENCE_H
