#ifndef RX_CORE_UTILITY_SWAP_H
#define RX_CORE_UTILITY_SWAP_H
#include "rx/core/utility/move.h"

namespace rx::utility {

template<typename T>
inline void swap(T &_lhs, T &_rhs) {
  T tmp{utility::move(_lhs)};
  _lhs = utility::move(_rhs);
  _rhs = utility::move(tmp);
}

} // namespace rx::utility

#endif // RX_CORE_UTILITY_SWAP_H