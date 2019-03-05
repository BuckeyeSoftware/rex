#ifndef RX_CORE_TRAITS_ADD_CONST_H
#define RX_CORE_TRAITS_ADD_CONST_H

#include <rx/core/traits/type_identity.h>

namespace rx::traits {

namespace detail {
  template<typename T>
  struct add_const : type_identity<const T> {};
} // namespace detail

template<typename T>
using add_const = typename detail::add_const<T>::type;

} // namespace rx::traits

#endif // RX_CORE_TRAITS_ADD_CONST_H
