#ifndef RX_CORE_ARRAY_H
#define RX_CORE_ARRAY_H
#include "rx/core/assert.h"

namespace Rx {

template<typename T>
struct Array;

template<typename T, Size E>
struct Array<T[E]> {
  template<typename... Ts>
  constexpr Array(Ts&&... _arguments);

  constexpr T& operator[](Size _index);
  constexpr const T& operator[](Size _index) const;

  constexpr T* data();
  constexpr const T* data() const;

  constexpr Size size() const;

private:
  T m_data[E];
};

/// @{
/// Operators for element-wise comparison of an Array
template<typename T, Size E>
inline bool operator==(const Array<T[E]>& _lhs, const Array<T[E]>& _rhs) {
  for (Size i = 0; i < E; i++) {
    if (_lhs[i] != _rhs[i]) {
      return false;
    }
  }
  return true;
}
template<typename T, Size E>
inline bool operator!=(const Array<T[E]>& _lhs, const Array<T[E]>& _rhs) {
  for (Size i = 0; i < E; i++) {
    if (_lhs[i] != _rhs[i]) {
      return true;
    }
  }
  return false;
}
/// @}

/// Deduction guide for \pre Array{Ts...} to become \pre Array<T[E]>.
template<typename T, typename... Ts>
Array(T, Ts...) -> Array<T[1 + sizeof...(Ts)]>;

template<typename T, Size E>
template<typename... Ts>
constexpr Array<T[E]>::Array(Ts&&... _arguments)
  : m_data{Utility::forward<Ts>(_arguments)...}
{
}

template<typename T, Size E>
constexpr T& Array<T[E]>::operator[](Size _index) {
  RX_ASSERT(_index < E, "out of bounds (%zu >= %zu)", _index, E);
  return m_data[_index];
}

template<typename T, Size E>
constexpr const T& Array<T[E]>::operator[](Size _index) const {
  RX_ASSERT(_index < E, "out of bounds (%zu >= %zu)", _index, E);
  return m_data[_index];
}

template<typename T, Size E>
constexpr T* Array<T[E]>::data() {
  return m_data;
}

template<typename T, Size E>
constexpr const T* Array<T[E]>::data() const {
  return m_data;
}

template<typename T, Size E>
constexpr Size Array<T[E]>::size() const {
  return E;
}

} // namespace Rx

#endif // RX_CORE_ARRAY_H
