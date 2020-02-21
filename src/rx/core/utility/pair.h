#ifndef RX_CORE_UTILITY_PAIR_H
#define RX_CORE_UTILITY_PAIR_H
#include "rx/core/utility/move.h"

namespace rx {

template<typename T1, typename T2>
struct pair {
  constexpr pair();
  constexpr pair(const T1& _first, const T2& _second);

  template<typename U1, typename U2>
  constexpr pair(U1&& first_, U2&& second_);

  template<typename U1, typename U2>
  constexpr pair(const pair<U1, U2>& _pair);

  template<typename U1, typename U2>
  constexpr pair(pair<U1, U2>&& pair_);

  pair(const pair& _pair);
  pair(pair&& _pair);

  T1 first;
  T2 second;
};

template<typename T1, typename T2>
inline constexpr pair<T1, T2>::pair()
  : first{}
  , second{}
{
}

template<typename T1, typename T2>
inline constexpr pair<T1, T2>::pair(const T1& _first, const T2& _second)
  : first{_first}
  , second{_second}
{
}

template<typename T1, typename T2>
template<typename U1, typename U2>
inline constexpr pair<T1, T2>::pair(U1&& first_, U2&& second_)
  : first{utility::move(first_)}
  , second{utility::move(second_)}
{
}

template<typename T1, typename T2>
template<typename U1, typename U2>
inline constexpr pair<T1, T2>::pair(const pair<U1, U2>& _pair)
  : first{_pair.first}
  , second{_pair.second}
{
}

template<typename T1, typename T2>
template<typename U1, typename U2>
inline constexpr pair<T1, T2>::pair(pair<U1, U2>&& pair_)
  : first{utility::move(pair_.first)}
  , second{utility::move(pair_.second)}
{
}

template<typename T1, typename T2>
inline pair<T1, T2>::pair(const pair&) = default;

template<typename T1, typename T2>
inline pair<T1, T2>::pair(pair&&) = default;

} // namespace rx

#endif // RX_CORE_UTILITY_PAIR_H
