#ifndef RX_CORE_CONCEPTS_INTEGRAL_H
#define RX_CORE_CONCEPTS_INTEGRAL_H
#include "rx/core/traits/is_same.h"
#include "rx/core/traits/remove_cv.h"

namespace Rx::Concepts {

template<typename T>
concept Integral =
     Traits::IS_SAME<T, bool>
  || Traits::IS_SAME<T, char>
  || Traits::IS_SAME<T, signed char>
  || Traits::IS_SAME<T, unsigned char>
  || Traits::IS_SAME<T, short>
  || Traits::IS_SAME<T, unsigned short>
  || Traits::IS_SAME<T, int>
  || Traits::IS_SAME<T, unsigned int>
  || Traits::IS_SAME<T, long>
  || Traits::IS_SAME<T, unsigned long>
  || Traits::IS_SAME<T, long long>
  || Traits::IS_SAME<T, unsigned long long>;

template<typename T>
concept SignedIntegral = Integral<T> && T(-1) < T(0);

template<typename T>
concept UnsignedIntegral = Integral<T> && T(-1) > T(0);

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_INTEGRAL_H
