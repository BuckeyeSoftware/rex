#ifndef RX_CORE_SPAN_H
#define RX_CORE_SPAN_H
#include "rx/core/types.h" // Size

namespace Rx {

template<typename T>
struct Span {
  constexpr Span(T* _data, Size _size);

  template<Size N>
  constexpr Span(T (&_data)[N]);

  constexpr T& operator[](Size _index) const;

  template<typename U>
  Span<U> cast() const;

  constexpr T* data() const;
  constexpr Size size() const;

private:
  T* m_data;
  Size m_size;
};

template<typename T>
inline constexpr Span<T>::Span(T* _data, Size _size)
  : m_data{_data}
  , m_size{_size}
{
}

template<typename T>
template<Size N>
inline constexpr Span<T>::Span(T (&_data)[N])
  : Span{_data, N}
{
}

template<typename T>
inline constexpr T& Span<T>::operator[](Size _index) const {
  return m_data[_index];
}

template<typename T>
template<typename U>
inline Span<U> Span<T>::cast() const {
  return {reinterpret_cast<U*>(m_data), (m_size * sizeof(T)) / sizeof(U)};
}

template<typename T>
inline constexpr T* Span<T>::data() const {
  return m_data;
}

template<typename T>
inline constexpr Size Span<T>::size() const {
  return m_size;
}

} // namespace Rx

#endif // RX_CORE_SPAN_H