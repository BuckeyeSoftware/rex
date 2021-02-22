#ifndef RX_CORE_UTILITY_EXCHANGE_H
#define RX_CORE_UTILITY_EXCHANGE_H
#include "rx/core/utility/move.h"
#include "rx/core/utility/forward.h"

#include "rx/core/hints/force_inline.h"

namespace Rx::Utility {

template<typename T, typename U = T>
RX_HINT_FORCE_INLINE T exchange(T& object_, U&& _new_value) {
  T old_value = move(object_);
  object_ = forward<U>(_new_value);
  return old_value;
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_EXCHANGE_H
