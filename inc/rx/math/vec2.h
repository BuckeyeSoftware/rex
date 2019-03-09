#ifndef RX_MATH_VEC2_H
#define RX_MATH_VEC2_H

#include <rx/core/types.h> // rx_size
#include <rx/core/format.h> // format
#include <rx/core/hash.h> // hash, hash_combine
#include <rx/core/assert.h> // RX_ASSERT

namespace rx::math {

template<typename T>
struct vec2 {
  constexpr vec2();
  constexpr vec2(T x, T y);
  T& operator[](rx_size i);
  const T& operator[](rx_size i) const;
  bool is_any(T value) const;
  union {
    struct { T x, y; };
    struct { T w, h; };
    struct { T r, g; };
    T v[2];
  };
};

using vec2f = vec2<rx_f32>;
using vec2i = vec2<rx_s32>;
using vec2z = vec2<rx_size>;

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

template<typename T>
inline bool vec2<T>::is_any(T value) const {
  return x == value || y == value;
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

template<typename T>
inline constexpr bool operator==(const vec2<T>& lhs, const vec2<T>& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

template<typename T>
inline constexpr bool operator!=(const vec2<T>& lhs, const vec2<T>& rhs) {
  return lhs.x != rhs.x || lhs.y != rhs.y;
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

  template<>
  struct hash<::rx::math::vec2f> {
    rx_size operator()(const ::rx::math::vec2f& _value) {
      const auto x{hash<rx_f32>{}(_value.x)};
      const auto y{hash<rx_f32>{}(_value.y)};
      return hash_combine(x, y);
    }
  };

  template<>
  struct hash<::rx::math::vec2i> {
    rx_size operator()(const ::rx::math::vec2i& _value) {
      const auto x{hash<rx_s32>{}(_value.x)};
      const auto y{hash<rx_s32>{}(_value.y)};
      return hash_combine(x, y);
    }
  };
} // namespace rx

#endif // RX_MATH_VEC2_H
