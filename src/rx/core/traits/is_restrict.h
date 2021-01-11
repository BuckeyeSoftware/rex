#ifndef RX_CORE_TRAITS_IS_RESTRICT_H
#define RX_CORE_TRAITS_IS_RESTRICT_H
#include "rx/core/hints/restrict.h"

namespace Rx::Traits {

template<typename T>
inline constexpr const bool IS_RESTRICT = false;

template<typename T>
inline constexpr const bool IS_RESTRICT<T *RX_HINT_RESTRICT> = true;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_IS_RESTRICT_H
