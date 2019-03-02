#ifndef RX_CORE_ALGORITHM_H
#define RX_CORE_ALGORITHM_H

#include <rx/core/traits.h> // move, forward

namespace rx {

template<typename T>
inline void swap(T &lhs, T &rhs) {
  T tmp{move(lhs)};
  lhs = move(rhs);
  rhs = move(tmp);
}

template<typename T>
inline T min(T a) {
  return a;
}

template<typename T, typename... Ts>
inline T min(T a, T b, Ts&&... args) {
  return min(a < b ? a : b, forward<Ts>(args)...);
}

template<typename T>
inline T max(T a) {
  return a;
}

template<typename T, typename... Ts>
inline T max(T a, T b, Ts&&... args) {
  return max(a > b ? a : b, forward<Ts>(args)...);
}

} // namespace rx

#endif // RX_CORE_ALGORITHM_H
