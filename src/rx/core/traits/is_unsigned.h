#ifndef RX_CORE_TRAITS_IS_UNSIGNED_H
#define RX_CORE_TRAITS_IS_UNSIGNED_H
#include "rx/core/traits/is_integral.h"

namespace rx::traits {

template<typename T>
inline constexpr bool is_unsigned = is_integral<T> && T(0) < T(-1);

} // namespace rx::traits

#endif // RX_CORE_TRAITS_IS_UNSIGNED_H
