#ifndef RX_MATH_VEC4_H
#define RX_MATH_VEC4_H

#include <rx/core/types.h> // rx_size
#include <rx/core/assert.h> // RX_ASSERT

namespace rx::math {

template<typename T>
struct vec4 {
  constexpr vec4();
  constexpr vec4(T x, T y, T z, T w);
  T& operator[](rx_size i);
  const T& operator[](rx_size i) const;
  union {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    T v[4];
  };
};

using vec4f = vec4<float>;

template<typename T>
inline constexpr vec4<T>::vec4()
  : x{0}
  , y{0}
  , z{0}
  , w{0}
{
}

template<typename T>
inline constexpr vec4<T>::vec4(T x, T y, T z, T w)
  : x{x}
  , y{y}
  , z{z}
  , w{w}
{
}

template<typename T>
inline T& vec4<T>::operator[](rx_size i) {
  RX_ASSERT(i < 4, "out of bounds");
  return v[i];
}

template<typename T>
inline const T& vec4<T>::operator[](rx_size i) const {
  RX_ASSERT(i < 4, "out of bounds");
  return v[i];
}

// (vec, vec)
template<typename T>
inline constexpr vec4<T> operator+(const vec4<T>& lhs, const vec4<T>& rhs) {
  return {lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z, lhs.w+rhs.w};
}

template<typename T>
inline constexpr vec4<T> operator-(const vec4<T>& lhs, const vec4<T>& rhs) {
  return {lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-rhs.w};
}

template<typename T>
inline constexpr vec4<T> operator*(const vec4<T>& lhs, const vec4<T>& rhs) {
  return {lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z, lhs.w*rhs.w};
}

template<typename T>
inline constexpr vec4<T> operator/(const vec4<T>& lhs, const vec4<T>& rhs) {
  return {lhs.x/rhs.x, lhs.y/rhs.y, lhs.z/rhs.z, lhs.w/rhs.w};
}

// (vec, scalar)
template<typename T>
inline constexpr vec4<T> operator+(const vec4<T>& lhs, T rhs) {
  return {lhs.x+rhs, lhs.y+rhs, lhs.z+rhs, lhs.w+rhs};
}

template<typename T>
inline constexpr vec4<T> operator-(const vec4<T>& lhs, T rhs) {
  return {lhs.x-rhs, lhs.y-rhs, lhs.z-rhs, lhs.w-rhs};
}

template<typename T>
inline constexpr vec4<T> operator*(const vec4<T>& lhs, T rhs) {
  return {lhs.x*rhs, lhs.y*rhs, lhs.z*rhs, lhs.w*rhs};
}

template<typename T>
inline constexpr vec4<T> operator/(const vec4<T>& lhs, T rhs) {
  return {lhs.x/rhs, lhs.y/rhs, lhs.z/rhs, lhs.w/rhs};
}

// (scalar, vec)
template<typename T>
inline constexpr vec4<T> operator+(T lhs, const vec4<T>& rhs) {
  return {lhs+rhs.x, lhs+rhs.y, lhs+rhs.z, lhs+rhs.w};
}

template<typename T>
inline constexpr vec4<T> operator-(T lhs, const vec4<T>& rhs) {
  return {lhs-rhs.x, lhs-rhs.y, lhs-rhs.z, lhs-rhs.w};
}

template<typename T>
inline constexpr vec4<T> operator*(T lhs, const vec4<T>& rhs) {
  return {lhs*rhs.x, lhs*rhs.y, lhs*rhs.z, lhs*rhs.w};
}

template<typename T>
inline constexpr vec4<T> operator/(T lhs, const vec4<T>& rhs) {
  return {lhs/rhs.x, lhs/rhs.y, lhs/rhs.z, lhs/rhs.w};
}

// dot product
template<typename T>
inline constexpr T dot(const vec4<T> &lhs, const vec4<T> &rhs) {
  return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z + lhs.w*rhs.w;
}

} // namespace rx::math

#endif // RX_MATH_VEC4_H
