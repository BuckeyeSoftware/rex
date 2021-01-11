#ifndef RX_CORE_TRAITS_UNDERLYING_TYPE_H
#define RX_CORE_TRAITS_UNDERLYING_TYPE_H
#include "rx/core/concepts/enum.h"

namespace Rx::Traits {

template<Concepts::Enum T>
using UnderlyingType = __underlying_type(T);

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_UNDERLYING_TYPE_H
