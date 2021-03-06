#ifndef RX_MATH_MAT3X4_H
#define RX_MATH_MAT3X4_H
#include "rx/math/vec4.h"

namespace Rx::Math {

template<typename T>
struct Quat;

template<typename T>
struct Mat3x3;

template<typename T>
struct Vec3;

template<typename T>
struct Mat3x4 {
  using Vec = Vec4<T>;

  constexpr Mat3x4() = default;
  constexpr Mat3x4(const Vec& _x, const Vec& _y, const Vec& _z);

  explicit Mat3x4(const Mat3x3<T>& _rotation, const Vec3<T>& _translation);
  explicit Mat3x4(const Quat<T>& _rotation, const Vec3<T>& _translation);
  explicit Mat3x4(const Vec3<T>& _scale, const Quat<T>& _rotation, const Vec3<T>& _translation);

  T* data();
  const T* data() const;

  constexpr Mat3x4 operator*(const Mat3x4& _mat) const;
  constexpr Mat3x4 operator+(const Mat3x4& _mat) const;

  constexpr Mat3x4 operator*(T _scalar) const;
  constexpr Mat3x4 operator+(T _scalar) const;

  constexpr Mat3x4& operator*=(const Mat3x4& _mat);
  constexpr Mat3x4& operator+=(const Mat3x4& _mat);

  constexpr Mat3x4& operator*=(T _scalar);
  constexpr Mat3x4& operator+=(T _scalar);

  Vec x, y, z;
};

using Mat3x4f = Mat3x4<Float32>;

template<typename T>
constexpr Mat3x4<T>::Mat3x4(const Vec& _x, const Vec& _y, const Vec& _z)
  : x{_x}
  , y{_y}
  , z{_z}
{
}

template<typename T>
T* Mat3x4<T>::data() {
  return x.data();
}

template<typename T>
const T* Mat3x4<T>::data() const {
  return x.data();
}

template<typename T>
constexpr Mat3x4<T> Mat3x4<T>::operator*(const Mat3x4& _mat) const {
  return {(_mat.x*x.x + _mat.y*x.y + _mat.z*x.z) + Vec{0, 0, 0, x.w},
          (_mat.x*y.x + _mat.y*y.y + _mat.z*y.z) + Vec{0, 0, 0, y.w},
          (_mat.x*z.x + _mat.y*z.y + _mat.z*z.z) + Vec{0, 0, 0, z.w}};
}

template<typename T>
constexpr Mat3x4<T> Mat3x4<T>::operator+(const Mat3x4& _mat) const {
  return {x + _mat.x, y + _mat.y, z + _mat.z};
}

template<typename T>
constexpr Mat3x4<T> Mat3x4<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar};
}

template<typename T>
constexpr Mat3x4<T> Mat3x4<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar};
}

template<typename T>
constexpr Mat3x4<T>& Mat3x4<T>::operator*=(const Mat3x4& _mat) {
  return *this = *this * _mat;
}

template<typename T>
constexpr Mat3x4<T>& Mat3x4<T>::operator+=(const Mat3x4& _mat) {
  return *this = *this + _mat;
}

template<typename T>
constexpr Mat3x4<T>& Mat3x4<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
constexpr Mat3x4<T>& Mat3x4<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename T>
Mat3x4<T> invert(const Mat3x4<T>& _mat);

} // namespace Rx::Math

#endif // RX_MATH_MAT3X4_H
