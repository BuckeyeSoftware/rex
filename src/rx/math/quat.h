#ifndef RX_MATH_QUAT_H
#define RX_MATH_QUAT_H
#include "rx/core/types.h"

namespace rx::math {

template<typename T>
struct mat3x3;

template<typename T>
struct mat3x4;

template<typename T>
struct quat {
  constexpr quat(T _x, T _y, T _z, T _w);

  explicit quat(const mat3x3<T>& _rotation);
  explicit quat(const mat3x4<T>& _rotation);

  constexpr quat operator-() const;

  constexpr quat operator*(const quat& _quat) const;
  constexpr quat operator+(const quat& _quat) const;
  constexpr quat operator-(const quat& _quat) const;

  constexpr quat& operator*=(const quat& _quat);
  constexpr quat& operator+=(const quat& _quat);
  constexpr quat& operator-=(const quat& _quat);

  constexpr quat operator*(T _scalar) const;
  constexpr quat operator+(T _scalar) const;
  constexpr quat operator-(T _scalar) const;

  constexpr quat& operator*=(T _scalar);
  constexpr quat& operator+=(T _scalar);
  constexpr quat& operator-=(T _scalar);

  T x, y, z, w;
};

using quatf = quat<rx_f32>;

template<typename T>
inline constexpr quat<T>::quat(T _x, T _y, T _z, T _w)
  : x{_x}
  , y{_y}
  , z{_z}
  , w{_w}
{
}

template<typename T>
inline constexpr quat<T> quat<T>::operator-() const {
  return {-x, -y, -z, w};
}

template<typename T>
inline constexpr quat<T> quat<T>::operator*(const quat& _quat) const {
  return {w * _quat.x + x * _quat.w - y * _quat.z + z * _quat.y,
          w * _quat.y + x * _quat.z + y * _quat.w - z * _quat.x,
          w * _quat.z - x * _quat.y + y * _quat.x + z * _quat.w,
          w * _quat.w - x * _quat.x - y * _quat.y - z * _quat.z};
}

template<typename T>
inline constexpr quat<T> quat<T>::operator+(const quat& _quat) const {
  return {x + _quat.x, y + _quat.y, z + _quat.z, w + _quat.w};
}

template<typename T>
inline constexpr quat<T> quat<T>::operator-(const quat& _quat) const {
  return {x - _quat.x, y - _quat.y, z - _quat.z, w - _quat.w};
}

template<typename T>
inline constexpr quat<T>& quat<T>::operator*=(const quat& _quat) {
  return *this = *this * _quat;
}

template<typename T>
inline constexpr quat<T>& quat<T>::operator+=(const quat& _quat) {
  return *this = *this + _quat;
}

template<typename T>
inline constexpr quat<T>& quat<T>::operator-=(const quat& _quat) {
  return *this = *this - _quat;
}

template<typename T>
inline constexpr quat<T> quat<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar, w * _scalar};
}

template<typename T>
inline constexpr quat<T> quat<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar, w + _scalar};
}

template<typename T>
inline constexpr quat<T> quat<T>::operator-(T _scalar) const {
  return {x - _scalar, y - _scalar, z - _scalar, w - _scalar};
}

template<typename T>
inline constexpr quat<T>& quat<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr quat<T>& quat<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename T>
inline constexpr quat<T>& quat<T>::operator-=(T _scalar) {
  return *this = *this - _scalar;
}

template<typename T>
inline constexpr T dot(const quat<T>& _lhs, const quat<T>& _rhs) {
  return _lhs.x*_rhs.x + _lhs.y*_rhs.y + _lhs.z*_rhs.z + _lhs.w*_rhs.w;
}

rx_f32 length(const quatf& _value);
quatf normalize(const quatf& _value);

} // namespace rx::math

#endif // RX_MATH_QUAT_H
