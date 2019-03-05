#ifndef RX_CORE_TRAITS_DECAY_H
#define RX_CORE_TRAITS_DECAY_H

#include <rx/core/traits/is_array.h>
#include <rx/core/traits/is_function.h>
#include <rx/core/traits/remove_reference.h>
#include <rx/core/traits/remove_extent.h>
#include <rx/core/traits/remove_cv.h>
#include <rx/core/traits/conditional.h>
#include <rx/core/traits/add_pointer.h>

namespace rx::traits {

namespace detail {
  template<typename T>
  struct decay {
    using base_type = remove_reference<T>;
    using type = conditional<
      is_array<base_type>,
      remove_extent<base_type>*,
      conditional<
        is_function<base_type>,
        add_pointer<base_type>,
        remove_cv<base_type>
      >
    >;
  };
} // namespace detail

template<typename T>
using decay = typename detail::decay<T>::type;

} // namespace rx::traits

#endif // RX_CORE_TRAITS_DECAY_H
