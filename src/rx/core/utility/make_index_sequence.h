#ifndef RX_CORE_UTILITY_MAKE_INDEX_SEQUENCE_H
#define RX_CORE_UTILITY_MAKE_INDEX_SEQUENCE_H

#include "rx/core/types.h" // rx_size
#include "rx/core/utility/make_integer_sequence.h"

namespace rx::utility {

template<rx_size N>
using make_index_sequence = make_integer_sequence<rx_size, N>;

template<typename... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

} // namespace rx::utility

#endif // RX_CORE_UTILITY_MAKE_INDEX_SEQUENCE_H
