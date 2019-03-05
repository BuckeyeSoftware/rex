#ifndef RX_CORE_TRAITS_ADD_CV_H
#define RX_CORE_TRAITS_ADD_CV_H

#include <rx/core/traits/type_identity.h>

namespace rx::traits {

namespace detail {
  template<typename T>
  struct add_cv : type_identity<const volatile T> {};
} // namespace detail

template<typename T>
using add_cv = typename detail::add_cv<T>::type;

} // namespace rx::traits

#endif // RX_CORE_TRAITS_ADD_CV_H
