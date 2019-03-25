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
  constexpr vec3(T _x, T _y, T _z);

  T& operator[](rx_size _i);
  const T& operator[](rx_size _i) const;

  bool is_any(T _value) const;
  bool is_all(T _value) const;

  const T* data() const;

  template<typename T2>
  constexpr vec3<T2> cast() const;

  constexpr vec3 operator-() const;

  constexpr vec3 operator*(const vec3<T>& _vec) const;
  constexpr vec3 operator/(const vec3<T>& _vec) const;
  constexpr vec3 operator+(const vec3<T>& _vec) const;
  constexpr vec3 operator-(const vec3<T>& _vec) const;

  constexpr vec3& operator*=(const vec3<T>& _vec);
  constexpr vec3& operator/=(const vec3<T>& _vec);
  constexpr vec3& operator+=(const vec3<T>& _vec);
  constexpr vec3& operator-=(const vec3<T>& _vec);

  constexpr vec3 operator*(T _scalar) const;
  constexpr vec3 operator/(T _scalar) const;
  constexpr vec3 operator+(T _scalar) const;
  constexpr vec3 operator-(T _scalar) const;

  constexpr vec3& operator*=(T _scalar);
  constexpr vec3& operator/=(T _scalar);
  constexpr vec3& operator+=(T _scalar);
  constexpr vec3& operator-=(T _scalar);

  template<typename U>
  friend constexpr vec3<U> operator*(U _scalar, const vec3<T>& _vec);
  template<typename U>
  friend constexpr vec3<U> operator/(U _scalar, const vec3<T>& _vec);
  template<typename U>
  friend constexpr vec3<U> operator+(U _scalar, const vec3<T>& _vec);
  template<typename U>
  friend constexpr vec3<U> operator-(U _scalar, const vec3<T>& _vec);

  template<typename U>
  friend constexpr bool operator==(const vec3<U>& _lhs, const vec3<U>& _rhs);
  template<typename U>
  friend constexpr bool operator!=(const vec3<U>& _lhs, const vec3<U>& _rhs);

  union {
    struct { T x, y, z; };
    struct { T w, h, d; };
    struct { T r, g, b; };
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
inline const T* vec3<T>::data() const {
  return array;
}

template<typename T>
template<typename T2>
inline constexpr vec3<T2> vec3<T>::cast() const {
  return {static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z)};
}

template<typename T>
inline constexpr vec3<T> vec3<T>::operator-() const {
  return {-x, -y, -z};
}

// (vec, vec)
template<typename T>
inline constexpr vec3<T> vec3<T>::operator*(const vec3<T>& _vec) const {
  return {x*_vec.x, y*_vec.y, z*_vec.z};
}

template<typename T>
inline constexpr vec3<T> vec3<T>::operator/(const vec3<T>& _vec) const {
  return {x/_vec.x, y/_vec.y, z/_vec.z};
}

template<typename T>
inline constexpr vec3<T> vec3<T>::operator+(const vec3<T>& _vec) const {
  return {x+_vec.x, y+_vec.y, z+_vec.z};
}

template<typename T>
inline constexpr vec3<T> vec3<T>::operator-(const vec3<T>& _vec) const {
  return {x-_vec.x, y-_vec.y, z-_vec.z};
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator*=(const vec3<T>& _vec) {
  return *this = *this * _vec;
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator/=(const vec3<T>& _vec) {
  return *this = *this / _vec;
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator+=(const vec3<T>& _vec) {
  return *this = *this + _vec;
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator-=(const vec3<T>& _vec) {
  return *this = *this - _vec;
}

// (vec, scalar)
template<typename T>
inline constexpr vec3<T> vec3<T>::operator*(T _scalar) const {
  return {x*_scalar, y*_scalar, z*_scalar};
}

template<typename T>
inline constexpr vec3<T> vec3<T>::operator/(T _scalar) const {
  return {x/_scalar, y/_scalar, z/_scalar};
}

template<typename T>
inline constexpr vec3<T> vec3<T>::operator+(T _scalar) const {
  return {x+_scalar, y+_scalar, z+_scalar};
}

template<typename T>
inline constexpr vec3<T> vec3<T>::operator-(T _scalar) const {
  return {x-_scalar, y-_scalar, z-_scalar};
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator/=(T _scalar) {
  return *this = *this / _scalar;
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename T>
inline constexpr vec3<T>& vec3<T>::operator-=(T _scalar) {
  return *this = *this - _scalar;
}

// (scalar, vec)
template<typename U>
inline constexpr vec3<U> operator*(U _scalar, const vec3<U>& _vec) {
  return {_scalar*_vec.x, _scalar*_vec.y, _scalar*_vec.z};
}

template<typename U>
inline constexpr vec3<U> operator/(U _scalar, const vec3<U>& _vec) {
  return {_scalar/_vec.x, _scalar/_vec.y, _scalar/_vec.z};
}

template<typename U>
inline constexpr vec3<U> operator+(U _scalar, const vec3<U>& _vec) {
  return {_scalar+_vec.x, _scalar+_vec.y, _scalar+_vec.z};
}

template<typename U>
inline constexpr vec3<U> operator-(U _scalar, const vec3<U>& _vec) {
  return {_scalar-_vec.x, _scalar-_vec.y, _scalar-_vec.z};
}

template<typename U>
inline constexpr bool operator==(const vec3<U>& _lhs, const vec3<U>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z;
}

template<typename U>
inline constexpr bool operator!=(const vec3<U>& _lhs, const vec3<U>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y || _lhs.z != _rhs.z;
}

// dot and cross product
template<typename T>
inline constexpr T dot(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return _lhs.x*_rhs.x + _lhs.y*_rhs.y + _lhs.z*_rhs.z;
}

template<typename T>
inline constexpr vec3<T> cross(const vec3<T>& _lhs, const vec3<T>& _rhs) {
  return {_lhs.y*_rhs.z - _rhs.y*_lhs.z, _lhs.z*_rhs.x - _rhs.z*_lhs.x, _lhs.x*_rhs.y - _rhs.x*_lhs.y};
}

rx_f32 length(const vec3f& _value);
vec3f normalize(const vec3f& _value);

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
