#ifndef RX_CORE_TRAITS_IS_LVALUE_REFERENCE_H
#define RX_CORE_TRAITS_IS_LVALUE_REFERENCE_H

namespace Rx::Traits {

template<typename T>
inline constexpr const bool IS_LVALUE_REFERENCE = false;

template<typename T>
inline constexpr const bool IS_LVALUE_REFERENCE<T&> = true;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_IS_LVALUE_REFERENCE_H
