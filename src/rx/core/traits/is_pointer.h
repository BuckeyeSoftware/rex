#ifndef RX_CORE_TRAITS_IS_POINTER_H
#define RX_CORE_TRAITS_IS_POINTER_H
#include "rx/core/traits/remove_cv.h"

namespace Rx::Traits {

namespace _ {
  template<typename T>
  inline constexpr const bool IS_POINTER = false;

  template<typename T>
  inline constexpr const bool IS_POINTER<T*> = true;
} // namespace _

template<typename T>
inline constexpr const bool IS_POINTER = _::IS_POINTER<RemoveCV<T>*>;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_IS_POINTER_H