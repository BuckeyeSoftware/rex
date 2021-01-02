#ifndef RX_CORE_UTILITY_DESTRUCT_H
#define RX_CORE_UTILITY_DESTRUCT_H
#include "rx/core/hints/force_inline.h"

namespace Rx::Utility {

template<typename T>
RX_HINT_FORCE_INLINE void destruct(void* _data) {
  reinterpret_cast<T*>(_data)->~T();
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_DESTRUCT_H
