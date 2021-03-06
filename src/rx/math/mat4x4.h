#ifndef RX_MATH_MAT4X4_H
#define RX_MATH_MAT4X4_H
#include "rx/math/vec3.h" // Vec3
#include "rx/math/vec4.h" // Vec4

namespace Rx::Math {

template<typename T>
struct Quat;

template<typename T>
struct Range;

template<typename T>
struct Mat4x4 {
  using Vec = Vec4<T>;

  constexpr Mat4x4();
  constexpr Mat4x4(const Vec& _x, const Vec& _y, const Vec& _z, const Vec& _w);

  T* data();
  const T* data() const;

  static Mat4x4 scale(const Vec3<T>& _scale);
  static Mat4x4 rotate(const Vec3<T>& _rotate);
  static Mat4x4 rotate(const Quat<T>& _rotate);
  static Mat4x4 translate(const Vec3<T>& _translate);

  constexpr Mat4x4 operator*(const Mat4x4& _mat) const;
  constexpr Mat4x4 operator+(const Mat4x4& _mat) const;

  constexpr Mat4x4 operator*(T _scalar) const;
  constexpr Mat4x4 operator+(T _scalar) const;

  constexpr Mat4x4& operator*=(const Mat4x4& _mat);
  constexpr Mat4x4& operator+=(const Mat4x4& _mat);

  constexpr Mat4x4& operator*=(T _scalar);
  constexpr Mat4x4& operator+=(T _scalar);

  template<typename U>
  friend constexpr bool operator==(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs);

  template<typename U>
  friend constexpr bool operator!=(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs);

  Vec x, y, z, w;
};

using Mat4x4f = Mat4x4<float>;

template<typename T>
constexpr Mat4x4<T>::Mat4x4()
  : x{1, 0, 0, 0}
  , y{0, 1, 0, 0}
  , z{0, 0, 1, 0}
  , w{0, 0, 0, 1}
{
}

template<typename T>
constexpr Mat4x4<T>::Mat4x4(const Vec& x, const Vec& y, const Vec& z, const Vec& w)
  : x{x}
  , y{y}
  , z{z}
  , w{w}
{
}

template<typename T>
T* Mat4x4<T>::data() {
  return x.data();
}

template<typename T>
const T* Mat4x4<T>::data() const {
  return x.data();
}

template<typename T>
Mat4x4<T> Mat4x4<T>::scale(const Vec3<T>& _scale) {
  return {{_scale.x, 0,        0,        0},
          {0,        _scale.y, 0,        0},
          {0,        0,        _scale.z, 0},
          {0,        0,        0,        1}};
}

template<typename T>
Mat4x4<T> Mat4x4<T>::translate(const Vec3<T>& _translate) {
  return {{1,            0,            0,            0},
          {0,            1,            0,            0},
          {0,            0,            1,            0},
          {_translate.x, _translate.y, _translate.z, 1}};

}

template<typename T>
constexpr Mat4x4<T> Mat4x4<T>::operator*(const Mat4x4& _mat) const {
  return {_mat.x*x.x + _mat.y*x.y + _mat.z*x.z + _mat.w*x.w,
          _mat.x*y.x + _mat.y*y.y + _mat.z*y.z + _mat.w*y.w,
          _mat.x*z.x + _mat.y*z.y + _mat.z*z.z + _mat.w*z.w,
          _mat.x*w.x + _mat.y*w.y + _mat.z*w.z + _mat.w*w.w};
}

template<typename T>
constexpr Mat4x4<T> Mat4x4<T>::operator+(const Mat4x4& _mat) const {
  return {x + _mat.x, y + _mat.y, z + _mat.z, w + _mat.w};
}

template<typename T>
constexpr Mat4x4<T> Mat4x4<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar, w * _scalar};
}

template<typename T>
constexpr Mat4x4<T> Mat4x4<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar, w + _scalar};
}

template<typename T>
constexpr Mat4x4<T>& Mat4x4<T>::operator*=(const Mat4x4& _mat) {
  return *this = *this * _mat;
}

template<typename T>
constexpr Mat4x4<T>& Mat4x4<T>::operator+=(const Mat4x4& _mat) {
  return *this = *this + _mat;
}

template<typename T>
constexpr Mat4x4<T>& Mat4x4<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
constexpr Mat4x4<T>& Mat4x4<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename U>
constexpr bool operator==(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs) {
  return _lhs.x == _rhs.x && _lhs.y == _rhs.y && _lhs.z == _rhs.z && _lhs.w == _rhs.w;
}

template<typename U>
constexpr bool operator!=(const Mat4x4<U>& _lhs, const Mat4x4<U>& _rhs) {
  return _lhs.x != _rhs.x || _lhs.y != _rhs.y || _lhs.z != _rhs.z || _lhs.w != _rhs.w;
}

// Free functions.
template<typename T>
constexpr Mat4x4<T> transpose(const Mat4x4<T>& _mat) {
  return {{_mat.x.x, _mat.y.x, _mat.z.x, _mat.w.x},
          {_mat.x.y, _mat.y.y, _mat.z.y, _mat.w.y},
          {_mat.x.z, _mat.y.z, _mat.z.z, _mat.w.z}};
}

template<typename T>
constexpr Vec3<T> transform_point(const Vec3<T>& _point, const Mat4x4<T>& _mat) {
  const Vec3<T>& w{_mat.w.x, _mat.w.y, _mat.w.z};
  return transform_vector(_point, _mat) + w;
}

template<typename T>
constexpr Vec3<T> transform_vector(const Vec3<T>& _vector, const Mat4x4<T>& _mat) {
  const Vec3<T>& x{_mat.x.x, _mat.x.y, _mat.x.z};
  const Vec3<T>& y{_mat.y.x, _mat.y.y, _mat.y.z};
  const Vec3<T>& z{_mat.z.x, _mat.z.y, _mat.z.z};
  return x * _vector.x + y * _vector.y + z * _vector.z;
}

template<typename T>
constexpr Vec4<T> transform_vector(const Vec4<T>& _vector, const Mat4x4<T>& _mat) {
  return (_mat.x * _vector.x) + (_mat.y * _vector.y) + (_mat.z * _vector.z) + (_mat.w * _vector.w);
}

template<typename T>
Mat4x4<T> invert(const Mat4x4<T>& _mat);

template<typename T>
Mat4x4<T> perspective(T _fov, const Range<T>& _planes, T _aspect);

} // namespace Rx::Math


namespace Rx::Hash {
template<>
struct Hasher<Math::Mat4x4f> {
  constexpr Size operator()(const Math::Mat4x4f& _value) const {
    const auto x = Hasher<Math::Vec4f>{}(_value.x);
    const auto y = Hasher<Math::Vec4f>{}(_value.y);
    const auto z = Hasher<Math::Vec4f>{}(_value.z);
    const auto w = Hasher<Math::Vec4f>{}(_value.w);
    return Hash::combine(Hash::combine(x, Hash::combine(y, z)), w);
  }
};

} // namespace Rx::Hash
#endif // RX_MATH_MAT4X4_H
