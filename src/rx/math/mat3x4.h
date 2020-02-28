#ifndef RX_MATH_MAT3X4_H
#define RX_MATH_MAT3X4_H
#include "rx/math/vec4.h"

namespace rx::math {

template<typename T>
struct quat;

template<typename T>
struct mat3x3;

template<typename T>
struct vec3;

template<typename T>
struct mat3x4 {
  using vec = vec4<T>;

  constexpr mat3x4() = default;
  constexpr mat3x4(const vec& _x, const vec& _y, const vec& _z);

  explicit mat3x4(const mat3x3<T>& _rotation, const vec3<T>& _translation);
  explicit mat3x4(const quat<T>& _rotation, const vec3<T>& _translation);
  explicit mat3x4(const vec3<T>& _scale, const quat<T>& _rotation, const vec3<T>& _translation);

  static mat3x4 invert(const mat3x4& _mat);

  T* data();
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
inline T* mat3x4<T>::data() {
  return x.data();
}

template<typename T>
inline const T* mat3x4<T>::data() const {
  return x.data();
}

template<typename T>
inline mat3x4<T> mat3x4<T>::invert(const mat3x4& _mat) {
  vec3<T> inverse_rotation_x{_mat.x.x, _mat.y.x, _mat.z.x};
  vec3<T> inverse_rotation_y{_mat.x.y, _mat.y.y, _mat.z.y};
  vec3<T> inverse_rotation_z{_mat.x.z, _mat.y.z, _mat.z.z};

  inverse_rotation_x /= dot(inverse_rotation_x, inverse_rotation_x);
  inverse_rotation_y /= dot(inverse_rotation_y, inverse_rotation_y);
  inverse_rotation_z /= dot(inverse_rotation_z, inverse_rotation_z);

  const vec3<T> translate{_mat.x.w, _mat.y.w, _mat.z.w};

  return {{inverse_rotation_x.x, inverse_rotation_x.y, inverse_rotation_x.z, -dot(inverse_rotation_x, translate)},
          {inverse_rotation_y.x, inverse_rotation_y.y, inverse_rotation_y.z, -dot(inverse_rotation_y, translate)},
          {inverse_rotation_z.x, inverse_rotation_z.y, inverse_rotation_z.z, -dot(inverse_rotation_z, translate)}};
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
