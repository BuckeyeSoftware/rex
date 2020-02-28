#ifndef RX_MATH_VEC2_H
#define RX_MATH_VEC2_H
#include "rx/core/types.h" // rx_size
#include "rx/core/format.h" // format
#include "rx/core/hash.h" // hash, hash_combine
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/algorithm/min.h" // algorithm::min
#include "rx/core/algorithm/max.h" // algorithm::max

#include "rx/core/math/sqrt.h" // sqrt

namespace rx::math {

template<typename T>
struct vec2 {
  constexpr vec2();
  constexpr vec2(T _x, T _y);

  T& operator[](rx_size _i);
  const T& operator[](rx_size _i) const;

  bool is_any(T _value) const;
  bool is_all(T _value) const;

  T area() const;
  T sum() const;

  T max_element() const;
  T min_element() const;

  template<typename F>
  vec2<T> map(F&& _fn) const;

  T* data();
  const T* data() const;

  template<typename T2>
  constexpr vec2<T2> cast() const;

  void operator+=(const vec2<T>& _v);
  void operator-=(const vec2<T>& _v);
  void operator*=(T _scalar);
  void operator/=(T _scalar);

  union {
    struct { T x, y; };
    struct { T w, h; };
    struct { T r, g; };
    struct { T u, v; };
    struct { T s, t; };
    T array[2];
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
inline constexpr vec2<T>::vec2(T _x, T _y)
  : x{_x}
  , y{_y}
{
}

template<typename T>
inline T& vec2<T>::operator[](rx_size _i) {
  RX_ASSERT(_i < 2, "out of bounds");
  return array[_i];
}

template<typename T>
inline const T& vec2<T>::operator[](rx_size _i) const {
  RX_ASSERT(_i < 2, "out of bounds");
  return array[_i];
}

template<typename T>
inline bool vec2<T>::is_any(T _value) const {
  return x == _value || y == _value;
}

template<typename T>
inline bool vec2<T>::is_all(T _value) const {
  return x == _value && y == _value;
}

template<typename T>
inline T vec2<T>::area() const {
  return x * y;
}

template<typename T>
inline T vec2<T>::sum() const {
  return x + y;
}

template<typename T>
inline T vec2<T>::max_element() const {
  return algorithm::max(x, y);
}

template<typename T>
inline T vec2<T>::min_element() const {
  return algorithm::min(x, y);
}

template<typename T>
template<typename F>
inline vec2<T> vec2<T>::map(F&& _fn) const {
  return { _fn(x), _fn(y) };
}

template<typename T>
inline T* vec2<T>::data() {
  return array;
}

template<typename T>
inline const T* vec2<T>::data() const {
  return array;
}

template<typename T>
template<typename T2>
inline constexpr vec2<T2> vec2<T>::cast() const {
  return {static_cast<T2>(x), static_cast<T2>(y)};
}

template<typename T>
inline void vec2<T>::operator+=(const vec2<T>& _v) {
  x += _v.x;
  y += _v.y;
}

template<typename T>
inline void vec2<T>::operator-=(const vec2<T>& _v) {
  x -= _v.x;
  y -= _v.y;
}

template<typename T>
inline void vec2<T>::operator*=(T _scalar) {
  x *= _scalar;
  y *= _scalar;
}

template<typename T>
inline void vec2<T>::operator/=(T _scalar) {
  x /= _scalar;
  y /= _scalar;
}

template<typename T>
inline constexpr bool operator==(const vec2<T>& _lhs, const vec2<T>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y;
}

template<typename T>
inline constexpr bool operator!=(const vec2<T>& _lhs, const vec2<T>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y;
}

template<typename T>
inline constexpr vec2<T> operator-(const vec2<T>& _v) {
  return {-_v.x, -_v.y};
}

template<typename T>
inline constexpr vec2<T> operator+(const vec2<T>& _a, const vec2<T>& _b) {
  return {_a.x + _b.x, _a.y + _b.y};
}

template<typename T>
inline constexpr vec2<T> operator-(const vec2<T>& _a, const vec2<T>& _b) {
  return {_a.x - _b.x, _a.y - _b.y};
}

template<typename T>
inline constexpr vec2<T> operator*(T _scalar, const vec2<T>& _v) {
  return {_scalar * _v.x, _scalar * _v.y};
}

template<typename T>
inline constexpr vec2<T> operator*(const vec2<T>& _v, T _scalar) {
  return _scalar * _v;
}

// NOTE: check
template<typename T>
inline constexpr vec2<T> operator-(const vec2<T>& _v, T _scalar) {
  return {_v.x - _scalar, _v.y - _scalar};
}

// NOTE: check
template<typename T>
inline constexpr vec2<T> operator+(const vec2<T>& _v, T _scalar) {
  return {_v.x + _scalar, _v.y + _scalar};
}

// NOTE: check
template<typename T>
inline constexpr vec2<T> operator/(T _scalar, const vec2<T>& _v) {
  return {_scalar / _v.x, _scalar / _v.y};
}

// NOTE: check
template<typename T>
inline constexpr vec2<T> operator/(const vec2<T>& _v, T _scalar) {
  return {_v.x / _scalar, _v.y / _scalar};
}

// NOTE: check
template<typename T>
inline constexpr vec2<T> operator/(const vec2<T>& _a, const vec2<T>& _b) {
  return {_a.x / _b.x, _a.y / _b.y};
}

// NOTE: check
template<typename T>
inline constexpr vec2<T> operator*(const vec2<T>& _a, const vec2<T>& _b) {
  return {_a.x * _b.x, _a.y * _b.y};
}

template<typename T>
inline constexpr bool operator<(const vec2<T>& _a, const vec2<T>& _b) {
  return _a.x < _b.x && _a.y < _b.y;
}

template<typename T>
inline constexpr bool operator>(const vec2<T>& _a, const vec2<T>& _b) {
  return _a.x > _b.x && _a.y > _b.y;
}

template<typename T>
inline constexpr T dot(const vec2<T>& _lhs, const vec2<T>& _rhs) {
  return _lhs.x * _rhs.x + _lhs.y * _rhs.y;
}

// Only defined for floating point
inline rx_f32 length(const vec2f& _value) {
  return sqrt(dot(_value, _value));
}

template<typename T>
inline rx_f32 distance(const vec2f& _a, const vec2f& _b) {
  return length(_a - _b);
}

inline vec2f normalize(const vec2f& _v) {
  return (1.0f / length(_v)) * _v;
}

} // namespace rx::math

namespace rx {
  template<>
  struct format_type<math::vec2f> {
    char scratch[format_size<rx_f32>::size*2 + sizeof "{, }" - 1];
    const char* operator()(const math::vec2f& _value);
  };

  template<>
  struct format_type<math::vec2i> {
    char scratch[format_size<rx_s32>::size*2 + sizeof "{, }" - 1];
    const char* operator()(const math::vec2i& _value);
  };

  template<>
  struct hash<math::vec2f> {
    rx_size operator()(const math::vec2f& _value) {
      const auto x{hash<rx_f32>{}(_value.x)};
      const auto y{hash<rx_f32>{}(_value.y)};
      return hash_combine(x, y);
    }
  };

  template<>
  struct hash<math::vec2i> {
    rx_size operator()(const math::vec2i& _value) {
      const auto x{hash<rx_s32>{}(_value.x)};
      const auto y{hash<rx_s32>{}(_value.y)};
      return hash_combine(x, y);
    }
  };

  template<>
  struct hash<math::vec2z> {
    rx_size operator()(const math::vec2z& _value) {
      const auto x{hash<rx_size>{}(_value.x)};
      const auto y{hash<rx_size>{}(_value.y)};
      return hash_combine(x, y);
    }
  };
} // namespace rx

#endif // RX_MATH_VEC2_H
