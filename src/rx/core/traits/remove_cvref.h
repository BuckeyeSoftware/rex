#ifndef RX_CORE_TRAITS_REMOVE_CVREF_H
#define RX_CORE_TRAITS_REMOVE_CVREF_H
#include "rx/core/traits/remove_cv.h"
#include "rx/core/traits/remove_reference.h"

namespace Rx::Traits {

template<typename T>
using RemoveCVRef = RemoveCV<RemoveReference<T>>;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_REMOVE_CVREF_H
