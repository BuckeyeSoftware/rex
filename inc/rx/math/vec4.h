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
  constexpr vec4(T _x, T _y, T _z, T _w);

  T& operator[](rx_size _i);
  const T& operator[](rx_size _i) const;

  bool is_any(T _value) const;
  bool is_all(T _value) const;

  const T* data() const;

  template<typename T2>
  vec4<T2> cast() const;

  constexpr vec4 operator*(const vec4<T>& _vec) const;
  constexpr vec4 operator/(const vec4<T>& _vec) const;
  constexpr vec4 operator+(const vec4<T>& _vec) const;
  constexpr vec4 operator-(const vec4<T>& _vec) const;

  constexpr vec4& operator*=(const vec4<T>& _vec) const;
  constexpr vec4& operator/=(const vec4<T>& _vec) const;
  constexpr vec4& operator+=(const vec4<T>& _vec) const;
  constexpr vec4& operator-=(const vec4<T>& _vec) const;

  constexpr vec4 operator*(T _scalar) const;
  constexpr vec4 operator/(T _scalar) const;
  constexpr vec4 operator+(T _scalar) const;
  constexpr vec4 operator-(T _scalar) const;

  constexpr vec4& operator*=(T _scalar) const;
  constexpr vec4& operator/=(T _scalar) const;
  constexpr vec4& operator+=(T _scalar) const;
  constexpr vec4& operator-=(T _scalar) const;

  template<typename U>
  friend constexpr vec4<U> operator*(U _scalar, const vec4<T>& _vec);
  template<typename U>
  friend constexpr vec4<U> operator/(U _scalar, const vec4<T>& _vec);
  template<typename U>
  friend constexpr vec4<U> operator+(U _scalar, const vec4<T>& _vec);
  template<typename U>
  friend constexpr vec4<U> operator-(U _scalar, const vec4<T>& _vec);

  template<typename U>
  friend constexpr bool operator==(const vec4<U>& _lhs, const vec4<U>& _rhs);
  template<typename U>
  friend constexpr bool operator!=(const vec4<U>& _lhs, const vec4<U>& _rhs);

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
inline constexpr vec4<T>::vec4(T _x, T _y, T _z, T _w)
  : x{_x}
  , y{_y}
  , z{_z}
  , w{_w}
{
}

template<typename T>
inline T& vec4<T>::operator[](rx_size _i) {
  RX_ASSERT(_i < 4, "out of bounds");
  return v[_i];
}

template<typename T>
inline const T& vec4<T>::operator[](rx_size _i) const {
  RX_ASSERT(_i < 4, "out of bounds");
  return v[_i];
}

template<typename T>
inline bool vec4<T>::is_any(T _value) const {
  return x == _value || y == _value || z == _value || w == _value;
}

template<typename T>
inline bool vec4<T>::is_all(T _value) const {
  return x == _value && y == _value && z == _value && w == _value;
}

template<typename T>
inline const T* vec4<T>::data() const {
  return v;
}

// (vec, vec)
template<typename T>
inline constexpr vec4<T> vec4<T>::operator*(const vec4<T>& _vec) const {
  return {x*_vec.x, y*_vec.y, z*_vec.z, w*_vec.w};
}

template<typename T>
inline constexpr vec4<T> vec4<T>::operator/(const vec4<T>& _vec) const {
  return {x/_vec.x, y/_vec.y, z/_vec.z, w/_vec.w};
}

template<typename T>
inline constexpr vec4<T> vec4<T>::operator+(const vec4<T>& _vec) const {
  return {x+_vec.x, y+_vec.y, z+_vec.z, w+_vec.w};
}

template<typename T>
inline constexpr vec4<T> vec4<T>::operator-(const vec4<T>& _vec) const {
  return {x-_vec.x, y-_vec.y, z-_vec.z, w-_vec.w};
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator*=(const vec4<T>& _vec) const {
  return *this = *this * _vec;
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator/=(const vec4<T>& _vec) const {
  return *this = *this / _vec;
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator+=(const vec4<T>& _vec) const {
  return *this = *this + _vec;
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator-=(const vec4<T>& _vec) const {
  return *this = *this - _vec;
}

// (vec, scalar)
template<typename T>
inline constexpr vec4<T> vec4<T>::operator*(T _scalar) const {
  return {x*_scalar, y*_scalar, z*_scalar, w*_scalar};
}

template<typename T>
inline constexpr vec4<T> vec4<T>::operator/(T _scalar) const {
  return {x/_scalar, y/_scalar, z/_scalar, w/_scalar};
}

template<typename T>
inline constexpr vec4<T> vec4<T>::operator+(T _scalar) const {
  return {x+_scalar, y+_scalar, z+_scalar, w+_scalar};
}

template<typename T>
inline constexpr vec4<T> vec4<T>::operator-(T _scalar) const {
  return {x-_scalar, y-_scalar, z-_scalar, w-_scalar};
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator*=(T _scalar) const {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator/=(T _scalar) const {
  return *this = *this / _scalar;
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator+=(T _scalar) const {
  return *this = *this + _scalar;
}

template<typename T>
inline constexpr vec4<T>& vec4<T>::operator-=(T _scalar) const {
  return *this = *this - _scalar;
}

// (scalar, vec)
template<typename U>
inline constexpr vec4<U> operator*(U _scalar, const vec4<U>& _vec) {
  return {_scalar*_vec.x, _scalar*_vec.y, _scalar*_vec.z, _scalar*_vec.w};
}

template<typename U>
inline constexpr vec4<U> operator/(U _scalar, const vec4<U>& _vec) {
  return {_scalar/_vec.x, _scalar/_vec.y, _scalar/_vec.z, _scalar/_vec.w};
}

template<typename U>
inline constexpr vec4<U> operator+(U _scalar, const vec4<U>& _vec) {
  return {_scalar+_vec.x, _scalar+_vec.y, _scalar+_vec.z, _scalar+_vec.w};
}

template<typename U>
inline constexpr vec4<U> operator-(U _scalar, const vec4<U>& _vec) {
  return {_scalar-_vec.x, _scalar-_vec.y, _scalar-_vec.z, _scalar-_vec.w};
}

template<typename U>
inline constexpr bool operator==(const vec4<U>& _lhs, const vec4<U>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z && _lhs.w == _rhs.w;
}

template<typename U>
inline constexpr bool operator!=(const vec4<U>& _lhs, const vec4<U>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y || _lhs.z != _rhs.z || _lhs.w != _rhs.w;
}

// dot product
template<typename T>
inline constexpr T dot(const vec4<T>& _lhs, const vec4<T>& _rhs) {
  return _lhs.x*_rhs.x + _lhs.y*_rhs.y + _lhs.z*_rhs.z + _lhs.w*_rhs.w;
}

} // namespace rx::math

namespace rx {
  template<>
  struct format<math::vec4f> {
    char scratch[format_size<rx_f32>::size*4 + sizeof "{,,,   }" - 1];
    const char* operator()(const math::vec4f& _value);
  };

  template<>
  struct format<math::vec4i> {
    char scratch[format_size<rx_s32>::size*4 + sizeof "{,,,   }" - 1];
    const char* operator()(const math::vec4i& _value);
  };

  template<>
  struct hash<math::vec4f> {
    rx_size operator()(const math::vec4f& _value) {
      const auto x{hash<rx_f32>{}(_value.x)};
      const auto y{hash<rx_f32>{}(_value.y)};
      const auto z{hash<rx_f32>{}(_value.z)};
      const auto w{hash<rx_f32>{}(_value.w)};
      return hash_combine(hash_combine(x, y), hash_combine(z, w));
    }
  };

  template<>
  struct hash<math::vec4i> {
    rx_size operator()(const math::vec4i& _value) {
      const auto x{hash<rx_s32>{}(_value.x)};
      const auto y{hash<rx_s32>{}(_value.y)};
      const auto z{hash<rx_s32>{}(_value.z)};
      const auto w{hash<rx_s32>{}(_value.w)};
      return hash_combine(hash_combine(x, y), hash_combine(z, w));
    }
  };
} // namespace rx

#endif // RX_MATH_VEC4_H
