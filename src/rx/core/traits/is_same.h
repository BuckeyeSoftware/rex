#ifndef RX_CORE_TRAITS_IS_SAME_H
#define RX_CORE_TRAITS_IS_SAME_H

namespace Rx::Traits {

template<typename T1, typename T2>
inline constexpr const bool IS_SAME = false;

template<typename T>
inline constexpr const bool IS_SAME<T, T> = true;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_IS_SAME_H
