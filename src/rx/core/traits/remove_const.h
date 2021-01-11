#ifndef RX_CORE_TRAITS_REMOVE_CONST_H
#define RX_CORE_TRAITS_REMOVE_CONST_H

namespace Rx::Traits {

namespace _ {
  template<typename T>
  struct RemoveConst { using Type = T; };
  template<typename T>
  struct RemoveConst<const T> { using Type = T; };
}

template<typename T>
using RemoveConst = typename _::RemoveConst<T>::Type;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_REMOVE_CONST_H
