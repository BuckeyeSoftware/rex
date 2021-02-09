#ifndef RX_CORE_TRAITS_REMOVE_EXTENT_H
#define RX_CORE_TRAITS_REMOVE_EXTENT_H
#include "rx/core/types.h" // Size

namespace Rx::Traits {

namespace _ {
  template<typename T>
  struct RemoveExtent { using Type = T; };
  template<typename T>
  struct RemoveExtent<T[]> { using Type = T; };
  template<typename T, Size E>
  struct RemoveExtent<T[E]> { using Type = T; };
} // namespace _

template<typename T>
using RemoveExtent = typename _::RemoveExtent<T>;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_REMOVE_EXTENT_H