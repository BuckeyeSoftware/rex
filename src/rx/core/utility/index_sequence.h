#ifndef RX_CORE_UTILITY_INDEX_SEQUENCE_H
#define RX_CORE_UTILITY_INDEX_SEQUENCE_H
#include "rx/core/types.h" // rx_size
#include "rx/core/utility/integer_sequence.h"

namespace rx::utility {

template<rx_size... Is>
using index_sequence = integer_sequence<rx_size, Is...>;

} // namespace rx::utility

#endif // RX_CORE_UTILITY_INDEX_SEQUENCE_H
