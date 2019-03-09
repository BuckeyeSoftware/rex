#ifndef RX_MATH_VEC3_H
#define RX_MATH_VEC3_H

#include <rx/core/types.h> // rx_size
#include <rx/core/format.h> // format
#include <rx/core/hash.h> // hash, hash_combine
#include <rx/core/assert.h> // RX_ASSERT

namespace rx::math {

template<typename T>
struct vec3 {
  constexpr vec3();
  constexpr vec3(T x, T y, T z);
  T& operator[](rx_size i);
  const T& operator[](rx_size i) const;
  bool is_any(T value) const;
  union {
    struct { T x, y, z; };
    struct { T w, h, d; };
    struct { T r, g, b; };
    T v[3];
  };
};

using vec3f = vec3<rx_f32>;
using vec3i = vec3<rx_s32>;
using vec3z = vec3<rx_size>;

template<typename T>
inline constexpr vec3<T>::vec3()
  : x{0}
  , y{0}
  , z{0}
{
}

template<typename T>
inline constexpr vec3<T>::vec3(T x, T y, T z)
  : x{x}
  , y{y}
  , z{z}
{
}

template<typename T>
inline T& vec3<T>::operator[](rx_size i) {
  RX_ASSERT(i < 3, "out of bounds");
  return v[i];
}

template<typename T>
inline const T& vec3<T>::operator[](rx_size i) const {
  RX_ASSERT(i < 3, "out of bounds");
  return v[i];
}

template<typename T>
inline bool vec3<T>::is_any(T value) const {
  return x == value || y == value || z == value;
}

// (vec, vec)
template<typename T>
inline constexpr vec3<T> operator+(const vec3<T>& lhs, const vec3<T>& rhs) {
  return {lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator-(const vec3<T>& lhs, const vec3<T>& rhs) {
  return {lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator*(const vec3<T>& lhs, const vec3<T>& rhs) {
  return {lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator/(const vec3<T>& lhs, const vec3<T>& rhs) {
  return {lhs.x/rhs.x, lhs.y/rhs.y, lhs.z/rhs.z};
}

// (vec, scalar)
template<typename T>
inline constexpr vec3<T> operator+(const vec3<T>& lhs, T rhs) {
  return {lhs.x+rhs, lhs.y+rhs, lhs.z+rhs};
}

template<typename T>
inline constexpr vec3<T> operator-(const vec3<T>& lhs, T rhs) {
  return {lhs.x-rhs, lhs.y-rhs, lhs.z-rhs};
}

template<typename T>
inline constexpr vec3<T> operator*(const vec3<T>& lhs, T rhs) {
  return {lhs.x*rhs, lhs.y*rhs, lhs.z*rhs};
}

template<typename T>
inline constexpr vec3<T> operator/(const vec3<T>& lhs, T rhs) {
  return {lhs.x/rhs, lhs.y/rhs, lhs.z/rhs};
}

// (scalar, vec)
template<typename T>
inline constexpr vec3<T> operator+(T lhs, const vec3<T>& rhs) {
  return {lhs+rhs.x, lhs+rhs.y, lhs+rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator-(T lhs, const vec3<T>& rhs) {
  return {lhs-rhs.x, lhs-rhs.y, lhs-rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator*(T lhs, const vec3<T>& rhs) {
  return {lhs*rhs.x, lhs*rhs.y, lhs*rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator/(T lhs, const vec3<T>& rhs) {
  return {lhs/rhs.x, lhs/rhs.y, lhs/rhs.z};
}

template<typename T>
inline constexpr bool operator==(const vec3<T>& lhs, const vec3<T>& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

template<typename T>
inline constexpr bool operator!=(const vec3<T>& lhs, const vec3<T>& rhs) {
  return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

// dot and cross product
template<typename T>
inline constexpr T dot(const vec3<T> &lhs, const vec3<T> &rhs) {
  return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z;
}

template<typename T>
inline constexpr vec3<T> cross(const vec3<T>& lhs, const vec3<T>& rhs) {
  return {lhs.y*rhs.z - rhs.y*lhs.z, lhs.z*rhs.x - rhs.z*lhs.x, lhs.x*rhs.y - rhs.x*lhs.y};
}

} // namespace rx::math

namespace rx {
  template<>
  struct format<math::vec3f> {
    char scratch[format_size<rx_f32>::size*3 + sizeof "{,,  }" - 1];
    const char* operator()(const math::vec3f& value);
  };

  template<>
  struct format<::rx::math::vec3i> {
    char scratch[format_size<rx_s32>::size*3 + sizeof "{,,  }" - 1];
    const char* operator()(const math::vec3i& value);
  };

  template<>
  struct hash<math::vec3f> {
    rx_size operator()(const math::vec3f& _value) {
      const auto x{hash<rx_f32>{}(_value.x)};
      const auto y{hash<rx_f32>{}(_value.y)};
      const auto z{hash<rx_f32>{}(_value.z)};
      return hash_combine(hash_combine(x, y), z);
    }
  };

  template<>
  struct hash<math::vec3i> {
    rx_size operator()(const math::vec3i& _value) {
      const auto x{hash<rx_s32>{}(_value.x)};
      const auto y{hash<rx_s32>{}(_value.y)};
      const auto z{hash<rx_s32>{}(_value.z)};
      return hash_combine(hash_combine(x, y), z);
    }
  };
} // namespace rx

#endif // RX_MATH_VEC3_H
