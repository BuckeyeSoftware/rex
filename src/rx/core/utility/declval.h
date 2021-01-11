#ifndef RX_CORE_UTILITY_DECLVAL_H
#define RX_CORE_UTILITY_DECLVAL_H
#include "rx/core/concepts/referenceable.h"

namespace Rx::Utility {

template<Concepts::Referenceable T>
T&& declval();

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_DECLVAL_H
