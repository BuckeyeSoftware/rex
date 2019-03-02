#ifndef RX_MATH_VEC2_H
#define RX_MATH_VEC2_H

#include <rx/core/types.h> // rx_size
#include <rx/core/format.h> // format
#include <rx/core/assert.h> // RX_ASSERT

namespace rx::math {

template<typename T>
struct vec2 {
  constexpr vec2();
  constexpr vec2(T x, T y);
  T& operator[](rx_size i);
  const T& operator[](rx_size i) const;
  union {
    struct { T x, y; };
    struct { T w, h; };
    struct { T r, g; };
    T v[2];
  };
};

using vec2f = vec2<rx_f32>;
using vec2i = vec2<rx_s32>;

template<typename T>
inline constexpr vec2<T>::vec2()
  : x{0}
  , y{0}
{
}

template<typename T>
inline constexpr vec2<T>::vec2(T x, T y)
  : x{x}
  , y{y}
{
}

template<typename T>
inline T& vec2<T>::operator[](rx_size i) {
  RX_ASSERT(i < 2, "out of bounds");
  return v[i];
}

template<typename T>
inline const T& vec2<T>::operator[](rx_size i) const {
  RX_ASSERT(i < 2, "out of bounds");
  return v[i];
}

// (vec, vec)
template<typename T>
inline constexpr vec2<T> operator+(const vec2<T>& lhs, const vec2<T>& rhs) {
  return {lhs.x+rhs.x, lhs.y+rhs.y};
}

template<typename T>
inline constexpr vec2<T> operator-(const vec2<T>& lhs, const vec2<T>& rhs) {
  return {lhs.x-rhs.x, lhs.y-rhs.y};
}

template<typename T>
inline constexpr vec2<T> operator*(const vec2<T>& lhs, const vec2<T>& rhs) {
  return {lhs.x*rhs.x, lhs.y*rhs.y};
}

template<typename T>
inline constexpr vec2<T> operator/(const vec2<T>& lhs, const vec2<T>& rhs) {
  return {lhs.x/rhs.x, lhs.y/rhs.y};
}

// (vec, scalar)
template<typename T>
inline constexpr vec2<T> operator+(const vec2<T>& lhs, T rhs) {
  return {lhs.x+rhs, lhs.y+rhs};
}

template<typename T>
inline constexpr vec2<T> operator-(const vec2<T>& lhs, T rhs) {
  return {lhs.x-rhs, lhs.y-rhs};
}

template<typename T>
inline constexpr vec2<T> operator*(const vec2<T>& lhs, T rhs) {
  return {lhs.x*rhs, lhs.y*rhs};
}

template<typename T>
inline constexpr vec2<T> operator/(const vec2<T>& lhs, T rhs) {
  return {lhs.x/rhs, lhs.y/rhs};
}

// (scalar, vec)
template<typename T>
inline constexpr vec2<T> operator+(T lhs, const vec2<T>& rhs) {
  return {lhs+rhs.x, lhs+rhs.y};
}

template<typename T>
inline constexpr vec2<T> operator-(T lhs, const vec2<T>& rhs) {
  return {lhs-rhs.x, lhs-rhs.y};
}

template<typename T>
inline constexpr vec2<T> operator*(T lhs, const vec2<T>& rhs) {
  return {lhs*rhs.x, lhs*rhs.y};
}

template<typename T>
inline constexpr vec2<T> operator/(T lhs, const vec2<T>& rhs) {
  return {lhs/rhs.x, lhs/rhs.y};
}

// dot product
template<typename T>
inline constexpr T dot(const vec2<T> &lhs, const vec2<T> &rhs) {
  return lhs.x*rhs.x + lhs.y*rhs.y;
}

} // namespace rx::math

namespace rx {
  template<>
  struct format<::rx::math::vec2f> {
    char scratch[format_size<rx_f32>::size*2 + sizeof "{, }" - 1];
    const char* operator()(const ::rx::math::vec2f& value);
  };

  template<>
  struct format<::rx::math::vec2i> {
    char scratch[format_size<rx_s32>::size*2 + sizeof "{, }" - 1];
    const char* operator()(const ::rx::math::vec2i& value);
  };
} // namespace rx

#endif // RX_MATH_VEC2_H
