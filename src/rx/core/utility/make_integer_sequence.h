#ifndef RX_CORE_UTILITY_MAKE_INTEGER_SEQUENCE_H
#define RX_CORE_UTILITY_MAKE_INTEGER_SEQUENCE_H
#include "rx/core/utility/integer_sequence.h"

namespace rx::utility {

namespace detail {
  template<typename T, T L, T I = 0, typename S = integer_sequence<T>>
  struct make_integer_sequence;

  template<typename T, T L, T I, T... Ns>
  struct make_integer_sequence<T, L, I, integer_sequence<T, Ns...>>
    : make_integer_sequence<T, L, I + 1, integer_sequence<T, Ns..., I>>
  {
  };

  template<typename T, T L, T... Ns>
  struct make_integer_sequence<T, L, L, integer_sequence<T, Ns...>>
    : integer_sequence<T, Ns...>
  {
  };
} // namespace detail

template<typename T, T N>
using make_integer_sequence = typename detail::make_integer_sequence<T, N>::type;

} // namespace rx::utility

#endif
