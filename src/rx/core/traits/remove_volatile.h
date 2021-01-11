#ifndef RX_CORE_TRAITS_REMOVE_VOLATILE_H
#define RX_CORE_TRAITS_REMOVE_VOLATILE_H

namespace Rx::Traits {

namespace _ {
  template<typename T>
  struct RemoveVolatile { using Type = T; };
  template<typename T>
  struct RemoveVolatile<volatile T> { using Type = T; };
}

template<typename T>
using RemoveVolatile = typename _::RemoveVolatile<T>::Type;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_REMOVE_VOLATILE_H
