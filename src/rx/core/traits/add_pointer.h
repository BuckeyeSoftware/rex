#ifndef RX_CORE_TRAITS_ADD_POINTER_H
#define RX_CORE_TRAITS_ADD_POINTER_H
#include "rx/core/traits/remove_reference.h"

namespace Rx::Traits {

namespace _ {
  template<typename T>
  struct Identity { using Type = T; };
  template<typename T>
  auto add_pointer(int) -> Identity<Traits::RemoveReference<T>*>;
  template<typename T>
  auto add_pointer(...) -> Identity<T>;
  template<typename T>
  struct AddPointer : decltype(add_pointer<T>(0)) { };
} // namespace _

template<typename T>
using AddPointer = typename _::AddPointer<T>::Type;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_ADD_POINTER_H