#ifndef RX_CORE_TRAITS_REMOVE_REFERENCE_H
#define RX_CORE_TRAITS_REMOVE_REFERENCE_H

namespace Rx::Traits {

namespace _ {
  template<typename T>
  struct RemoveReference { using Type = T; };
  template<typename T>
  struct RemoveReference<T&> { using Type = T; };
  template<typename T>
  struct RemoveReference<T&&> { using Type = T; };
}

template<typename T>
using RemoveReference = typename _::RemoveReference<T>::Type;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_REMOVE_REFERENCE_H
