#ifndef RX_CORE_TRAITS_REMOVE_CONST_VOLATILE_H
#define RX_CORE_TRAITS_REMOVE_CONST_VOLATILE_H
#include "rx/core/traits/remove_const.h"
#include "rx/core/traits/remove_volatile.h"

namespace Rx::Traits {

template<typename T>
using RemoveCV = RemoveVolatile<RemoveConst<T>>;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_REMOVE_CV_H
