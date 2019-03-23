#ifndef RX_MATH_MAT3X4_H
#define RX_MATH_MAT3X4_H

#include <rx/math/vec3.h>
#include <rx/math/vec4.h>
#include <rx/math/mat3x4.h>

namespace rx::math {

template<typename T>
struct mat3x4 {
  using vec = vec4<T>;

  constexpr mat3x4() = default;
  constexpr mat3x4(const vec& _x, const vec& _y, const vec& _z);
  constexpr mat3x4(const mat3x4<T>& _rotation, const vec3<T>& _translation)

  const T* data() const;

  constexpr mat3x4 operator*(const mat3x4& _mat) const;
  constexpr mat3x4 operator+(const mat3x4& _mat) const;

  constexpr mat3x4 operator*(T _scalar) const;
  constexpr mat3x4 operator+(T _scalar) const;

  constexpr mat3x4& operator*=(const mat3x4& _mat);
  constexpr mat3x4& operator+=(const mat3x4& _mat);

  constexpr mat3x4& operator*=(T _scalar);
  constexpr mat3x4& operator+=(T _scalar);

  vec x, y, z;
};

using mat3x4f = mat3x4<rx_f32>;

template<typename T>
inline constexpr mat3x4<T>::mat3x4(const vec& _x, const vec& _y, const vec& _z)
  : x{_x}
  , y{_y}
  , z{_z}
{
}

template<typename T>
inline constexpr mat3x4<T>::mat3x4(const mat3x4<T>& _rotation, const vec3<T>& _translation)
  : x{_rotation.x.x, _rotation.x.y, _rotation.x.z, _translation.x}
  , y{_rotation.y.x, _rotation.y.y, _rotation.y.z, _translation.y}
  , z{_rotation.z.x, _rotation.z.y, _rotation.z.z, _translation.z}
{
}

template<typename T>
inline const T* mat3x4<T>::data() const {
  return x.data();
}

template<typename T>
inline constexpr mat3x4<T> mat3x4<T>::operator*(const mat3x4& _mat) const {
  return {(_mat.x*x.x + _mat.y*x.y + _mat.z*x.z) + vec{0, 0, 0, x.w},
          (_mat.x*y.x + _mat.y*y.y + _mat.z*y.z) + vec{0, 0, 0, y.w},
          (_mat.x*z.x + _mat.y*z.y + _mat.z*z.z) + vec{0, 0, 0, z.w}};
}

template<typename T>
inline constexpr mat3x4<T> mat3x4<T>::operator+(const mat3x4& _mat) const {
  return {x + _mat.x, y + _mat.y, z + _mat.z};
}

template<typename T>
inline constexpr mat3x4<T> mat3x4<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar};
}

template<typename T>
inline constexpr mat3x4<T> mat3x4<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar};
}

template<typename T>
inline constexpr mat3x4<T>& mat3x4<T>::operator*=(const mat3x4& _mat) {
  return *this = *this * _mat;
}

template<typename T>
inline constexpr mat3x4<T>& mat3x4<T>::operator+=(const mat3x4& _mat) {
  return *this = *this + _mat;
}

template<typename T>
inline constexpr mat3x4<T>& mat3x4<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr mat3x4<T>& mat3x4<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

} // namespace rx::math

#endif // RX_MATH_MAT3X4_H