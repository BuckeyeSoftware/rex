#ifndef RX_MATH_VEC4_H
#define RX_MATH_VEC4_H

#include <rx/core/types.h> // rx_size
#include <rx/core/format.h> // format
#include <rx/core/hash.h> // hash, hash_combine
#include <rx/core/assert.h> // RX_ASSERT

namespace rx::math {

template<typename T>
struct vec4 {
  constexpr vec4();
  constexpr vec4(T x, T y, T z, T w);
  T& operator[](rx_size i);
  const T& operator[](rx_size i) const;
  bool is_any(T value) const;
  union {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    T v[4];
  };
};

using vec4f = vec4<rx_f32>;
using vec4i = vec4<rx_s32>;
using vec4z = vec4<rx_size>;

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

template<typename T>
inline bool vec4<T>::is_any(T value) const {
  return x == value || y == value || z == value || w == value;
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

template<typename T>
inline constexpr bool operator==(const vec4<T>& lhs, const vec4<T>& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

template<typename T>
inline constexpr bool operator!=(const vec4<T>& lhs, const vec4<T>& rhs) {
  return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

// dot product
template<typename T>
inline constexpr T dot(const vec4<T> &lhs, const vec4<T> &rhs) {
  return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z + lhs.w*rhs.w;
}

} // namespace rx::math

namespace rx {
  template<>
  struct format<::rx::math::vec4f> {
    char scratch[format_size<rx_f32>::size*4 + sizeof "{,,,   }" - 1];
    const char* operator()(const ::rx::math::vec4f& value);
  };

  template<>
  struct format<::rx::math::vec4i> {
    char scratch[format_size<rx_s32>::size*4 + sizeof "{,,,   }" - 1];
    const char* operator()(const ::rx::math::vec4i& value);
  };

  template<>
  struct hash<::rx::math::vec4f> {
    rx_size operator()(const ::rx::math::vec4f& _value) {
      const auto x{hash<rx_f32>{}(_value.x)};
      const auto y{hash<rx_f32>{}(_value.y)};
      const auto z{hash<rx_f32>{}(_value.z)};
      const auto w{hash<rx_f32>{}(_value.w)};
      return hash_combine(hash_combine(x, y), hash_combine(z, w));
    }
  };

  template<>
  struct hash<::rx::math::vec4i> {
    rx_size operator()(const ::rx::math::vec4i& _value) {
      const auto x{hash<rx_s32>{}(_value.x)};
      const auto y{hash<rx_s32>{}(_value.y)};
      const auto z{hash<rx_s32>{}(_value.z)};
      const auto w{hash<rx_s32>{}(_value.w)};
      return hash_combine(hash_combine(x, y), hash_combine(z, w));
    }
  };
} // namespace rx

#endif // RX_MATH_VEC4_H
