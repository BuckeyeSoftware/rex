#ifndef RX_MATH_VEC3_H
#define RX_MATH_VEC3_H
#include "rx/core/types.h" // rx_size
#include "rx/core/format.h" // format
#include "rx/core/hash.h" // hash, hash_combine
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/math/sqrt.h"
#include "rx/core/math/sign.h"
#include "rx/core/math/abs.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace rx::math {

template<typename T>
struct vec3 {
  constexpr vec3();
  constexpr vec3(T _x, T _y, T _z);

  T& operator[](rx_size _i);
  const T& operator[](rx_size _i) const;

  bool is_any(T _value) const;
  bool is_all(T _value) const;

  T area() const;
  T sum() const;

  T max_element() const;
  T min_element() const;

  template<typename F>
  vec3<T> map(F&& _fn) const;

  T* data();
  const T* data() const;

  template<typename T2>
  constexpr vec3<T2> cast() const;

  void operator+=(const vec3<T>& _v);
  void operator-=(const vec3<T>& _v);
  void operator*=(T _scalar);
  void operator/=(T _scalar);

  // TODO: check
  void operator*=(const vec3<T>& _v);

  union {
    struct { T x, y, z; };
    struct { T w, h, d; };
    struct { T r, g, b; };
    struct { T s, t, p; };
    T array[3];
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
inline constexpr vec3<T>::vec3(T _x, T _y, T _z)
  : x{_x}
  , y{_y}
  , z{_z}
{
}

template<typename T>
inline T& vec3<T>::operator[](rx_size _i) {
  RX_ASSERT(_i < 3, "out of bounds");
  return array[_i];
}

template<typename T>
inline const T& vec3<T>::operator[](rx_size _i) const {
  RX_ASSERT(_i < 3, "out of bounds");
  return array[_i];
}

template<typename T>
inline bool vec3<T>::is_any(T _value) const {
  return x == _value || y == _value || z == _value;
}

template<typename T>
inline bool vec3<T>::is_all(T _value) const {
  return x == _value && y == _value && z == _value;
}

template<typename T>
inline T vec3<T>::area() const {
  return x * y * z;
}

template<typename T>
inline T vec3<T>::sum() const {
  return x + y + z;
}

template<typename T>
inline T vec3<T>::max_element() const {
  return algorithm::max(x, y, z);
}

template<typename T>
inline T vec3<T>::min_element() const {
  return algorithm::min(x, y, z);
}

template<typename T>
template<typename F>
inline vec3<T> vec3<T>::map(F&& _fn) const {
  return { _fn(x), _fn(y), _fn(z) };
}

template<typename T>
inline T* vec3<T>::data() {
  return array;
}

template<typename T>
inline const T* vec3<T>::data() const {
  return array;
}

template<typename T>
template<typename T2>
inline constexpr vec3<T2> vec3<T>::cast() const {
  return {static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z)};
}

template<typename T>
inline void vec3<T>::operator+=(const vec3<T>& _v) {
  x += _v.x;
  y += _v.y;
  z += _v.z;
}

template<typename T>
inline void vec3<T>::operator-=(const vec3<T>& _v) {
  x -= _v.x;
  y -= _v.y;
  z -= _v.z;
}

template<typename T>
inline void vec3<T>::operator*=(T _scalar) {
  x *= _scalar;
  y *= _scalar;
  z *= _scalar;
}

template<typename T>
inline void vec3<T>::operator/=(T _scalar) {
  x /= _scalar;
  y /= _scalar;
  z /= _scalar;
}

// TODO: check
template<typename T>
inline void vec3<T>::operator*=(const vec3<T>& _v) {
  x *= _v.x;
  y *= _v.y;
  z *= _v.z;
}

template<typename T>
inline constexpr bool operator==(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z;
}

template<typename T>
inline constexpr bool operator!=(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y || _lhs.z != _rhs.z;
}

template<typename T>
inline constexpr vec3<T> operator-(const vec3<T>& _v) {
  return {-_v.x, -_v.y, -_v.z};
}

template<typename T>
inline constexpr vec3<T> operator+(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return {_lhs.x + _rhs.x, _lhs.y + _rhs.y, _lhs.z + _rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator-(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return {_lhs.x - _rhs.x, _lhs.y - _rhs.y, _lhs.z - _rhs.z};
}

template<typename T>
inline constexpr vec3<T> operator*(T _scalar, const vec3<T>& _v) {
  return {_scalar * _v.x, _scalar * _v.y, _scalar * _v.z};
}

template<typename T>
inline constexpr vec3<T> operator*(const vec3<T>& _v, T _scalar) {
  return _scalar * _v;
}

// NOTE: check
template<typename T>
inline constexpr vec3<T> operator-(const vec3<T>& _v, T _scalar) {
  return {_v.x - _scalar, _v.y - _scalar, _v.z - _scalar};
}

// NOTE: check
template<typename T>
inline constexpr vec3<T> operator+(const vec3<T>& _v, T _scalar) {
  return {_v.x + _scalar, _v.y + _scalar, _v.z + _scalar};
}

// NOTE: check
template<typename T>
inline constexpr vec3<T> operator/(T _scalar, const vec3<T>& _v) {
  return {_scalar / _v.x, _scalar / _v.y, _scalar / _v.z};
}

// NOTE: check
template<typename T>
inline constexpr vec3<T> operator/(const vec3<T>& _v, T _scalar) {
  return {_v.x / _scalar, _v.y / _scalar, _v.z / _scalar};
}

// NOTE: check
template<typename T>
inline constexpr vec3<T> operator/(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return {_lhs.x / _rhs.x, _lhs.y / _rhs.y, _lhs.z / _rhs.z};
}

// NOTE: check
template<typename T>
inline constexpr vec3<T> operator*(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return {_lhs.x * _rhs.x, _lhs.y * _rhs.y, _lhs.z * _rhs.z};
}

template<typename T>
inline constexpr bool operator<(const vec3<T>& _a, const vec3<T>& _b) {
  return _a.x < _b.x && _a.y < _b.y && _a.z < _b.z;
}

template<typename T>
inline constexpr bool operator>(const vec3<T>& _a, const vec3<T>& _b) {
  return _a.x > _b.x && _a.y > _b.y && _a.z > _b.z;
}

// Functions.
template<typename T>
inline constexpr T dot(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return _lhs.x * _rhs.x + _lhs.y * _rhs.y + _lhs.z * _rhs.z;
}

template<typename T>
inline constexpr vec3<T> cross(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return {_lhs.y * _rhs.z - _rhs.y * _lhs.z,
          _lhs.z * _rhs.x - _rhs.z * _lhs.x,
          _lhs.x * _rhs.y - _rhs.x * _lhs.y};
}

// Compute the determinant of a matrix whose columns are three given vectors.
template<typename T>
inline constexpr T det(const vec3<T>& _a, const vec3<T>& _b, const vec3<T>& _c) {
  return dot(_a, cross(_b, _c));
}

// Compute minimum vector between two vectors (per-element).
template<typename T>
inline constexpr vec3<T> min(const vec3<T>& _a, const vec3<T>& _b) {
  return {algorithm::min(_a.x, _b.x), algorithm::min(_a.y, _b.y), algorithm::min(_a.z, _b.z)};
}

// Compute maximum vector between two vectors (per-element).
template<typename T>
inline constexpr vec3<T> max(const vec3<T>& _a, const vec3<T>& _b) {
  return {algorithm::max(_a.x, _b.x), algorithm::max(_a.y, _b.y), algorithm::max(_a.z, _b.z)};
}

// Compute absolute vector of a given vectgor (per-element).
template<typename T>
inline vec3<T> abs(const vec3<T>& _v) {
  return {abs(_v.x), abs(_v.y), abs(_v.y)};
}

// Only defined for floating point
inline rx_f32 length_squared(const vec3f& _v) {
  return dot(_v, _v);
}

inline rx_f32 length(const vec3f& _v) {
  return sqrt(length_squared(_v));
}

inline vec3f normalize(const vec3f& _v) {
  return (1.0f / length(_v)) * _v;
}

// Compute euclidean distance between two points.
inline rx_f32 distance(const vec3f& _a, const vec3f& _b) {
  return length(_a - _b);
}

// Compute squared distance between two points.
inline rx_f32 distance_squared(const vec3f& _a, const vec3f& _b) {
  return length_squared(_a - _b);
}

// Compute triangle area.
inline rx_f32 area(const vec3f& _a, const vec3f& _b, const vec3f& _c) {
  return 0.5f * length(cross(_b - _a, _c - _a));
}

// Compute squared triangle area.
inline rx_f32 squared_area(const vec3f& _a, const vec3f& _b, const vec3f& _c) {
  return 0.25f * length_squared(cross(_b - _a, _c - _a));
}

// Compute tetrahedron volume.
inline rx_f32 volume(const vec3f& _a, const vec3f& _b, const vec3f& _c,
  const vec3f& _d)
{
  const rx_f32 volume{det(_b - _a, _c - _a, _d - _a)};
  return sign(volume) * (1.0f / 6.0f) * volume;
}

// Compute squared tetrahdron volume.
inline rx_f32 volume_squared(const vec3f& _a, const vec3f& _b, const vec3f& _c,
  const vec3f& _d)
{
  const rx_f32 result{volume(_a, _b, _c, _d)};
  return result * result;
}

// Find a perpendicular vector to a vector.
inline vec3f perp(const vec3f& _v) {
  // Suppose vector a has all equal components and is a unit vector: a = (s, s, s)
  // Then 3*s*s = 1, s = sqrt(1/3) = 0.57735. This means at least one component of
  // a unit vector must be greater than or equal to 0.557735.
  if (abs(_v.x) >= 0.557735f) {
    return {_v.y, -_v.x, 0.0f};
  }

  return {0.0f, _v.z, -_v.x};
}

} // namespace rx::math

namespace rx {
  template<>
  struct format_type<math::vec3f> {
    char scratch[format_size<rx_f32>::size*3 + sizeof "{,,  }" - 1];
    const char* operator()(const math::vec3f& _value);
  };

  template<>
  struct format_type<::rx::math::vec3i> {
    char scratch[format_size<rx_s32>::size*3 + sizeof "{,,  }" - 1];
    const char* operator()(const math::vec3i& _value);
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
