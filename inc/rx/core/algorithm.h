#ifndef RX_CORE_ALGORITHM_H
#define RX_CORE_ALGORITHM_H

#include <rx/core/traits.h> // move

namespace rx {

template<typename T>
inline void swap(T &lhs, T &rhs) {
  T tmp{move(lhs)};
  lhs = move(rhs);
  rhs = move(tmp);
}

} // namespace rx

#endif // RX_CORE_ALGORITHM_H
