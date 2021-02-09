#ifndef RX_CORE_TRAITS_IS_ARRAY_H
#define RX_CORE_TRAITS_IS_ARRAY_H
#include "rx/core/types.h" // Size

namespace Rx::Traits {

template<typename>
inline constexpr const auto IS_ARRAY = false;

template<typename T>
inline constexpr const auto IS_ARRAY<T[]> = true;

template<typename T, Size E>
inline constexpr const auto IS_ARRAY<T[E]> = true;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_IS_ARRAY_H