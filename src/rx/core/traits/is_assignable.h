#ifndef RX_CORE_TRAITS_IS_ASSIGNABLE_H
#define RX_CORE_TRAITS_IS_ASSIGNABLE_H
#include "rx/core/traits/is_void.h"

namespace rx::traits {

namespace detail {
} // namespace detail

template<typename T, typename A>
inline constexpr bool is_assignable{false};

} // namespace rx::traits

#endif