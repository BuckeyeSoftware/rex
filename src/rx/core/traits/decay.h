#ifndef RX_CORE_TRAITS_DECAY_H
#define RX_CORE_TRAITS_DECAY_H
#include "rx/core/traits/remove_reference.h"
#include "rx/core/traits/remove_extent.h"
#include "rx/core/traits/remove_cv.h"
#include "rx/core/traits/add_pointer.h"
#include "rx/core/traits/is_array.h"
#include "rx/core/traits/is_function.h"
#include "rx/core/traits/conditional.h"

namespace Rx::Traits {

namespace _ {
  template<typename T>
  struct Decay {
    using Base = Traits::RemoveReference<T>;
    using Type = Traits::Conditional<
      IS_ARRAY<Base>,
      Traits::RemoveExtent<Base>*,
      Traits::Conditional<
        IS_FUNCTION<Base>,
        Traits::AddPointer<Base>,
        Traits::RemoveCV<Base>>>;
  };
} // namespace _

// Applies lvalue-to-rvalue, array-to-pointer, and function-to-pointer
// implicit conversions to the type |T|, removes cv-qualifiers, and produces
// formally:
//
// * If |T| names the type "array of U" or "reference to array of U", type is U*
//
// * Otherwise, if |T| is a function type |F| or a reference thereto, type is
//   AddPointer<F>.
//
// * Otherwise, the type is RemoveCV<RemoveReference<T>>.
//
// These conversions model type conversions applied to all function arguments
// when passed by value.
template<typename T>
using Decay = typename _::Decay<T>::Type;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_DECAY_H