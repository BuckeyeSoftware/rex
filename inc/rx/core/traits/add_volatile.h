#ifndef RX_CORE_TRAITS_ADD_VOLATILE_H
#define RX_CORE_TRAITS_ADD_VOLATILE_H

#include <rx/core/traits/type_identity.h>

namespace rx::traits {

namespace detail {
  template<typename T>
  struct add_volatile : type_identity<volatile T> {};
} // namespace detail

template<typename T>
using add_volatile = typename detail::add_volatile<T>::type;

} // namespace rx::traits

#endif // RX_CORE_TRAITS_ADD_VOLATILE_H
