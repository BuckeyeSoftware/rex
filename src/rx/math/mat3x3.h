#ifndef RX_MATH_MAT3X3_H
#define RX_MATH_MAT3X3_H
#include "rx/core/math/cos.h" // cos
#include "rx/core/math/sin.h" // sin

#include "rx/math/vec3.h" // vec3
#include "rx/math/trig.h" // deg_to_rad

namespace rx::math {

template<typename T>
struct quat;

template<typename T>
struct mat3x3 {
  using vec = vec3<T>;

  constexpr mat3x3();
  constexpr mat3x3(const vec& _x, const vec& _y, const vec& _z);

  explicit mat3x3(const quat<T>& _rotation);
  explicit mat3x3(const vec3<T>& _scale, const quat<T>& _rotation);

  T* data();
  const T* data() const;

  static constexpr mat3x3 scale(const vec3<T>& _scale);
  static constexpr mat3x3 rotate(const vec3<T>& _rotate);
  static constexpr mat3x3 translate(const vec3<T>& _translate);
  static constexpr mat3x3 transpose(const mat3x3& _mat);

  constexpr mat3x3 operator*(const mat3x3& _mat) const;
  constexpr mat3x3 operator+(const mat3x3& _mat) const;

  constexpr mat3x3 operator*(T _scalar) const;
  constexpr mat3x3 operator+(T _scalar) const;

  constexpr mat3x3& operator*=(const mat3x3& _mat);
  constexpr mat3x3& operator+=(const mat3x3& _mat);

  constexpr mat3x3& operator*=(T _scalar);
  constexpr mat3x3& operator+=(T _scalar);

  const vec3f& operator[](rx_size _index) const;
  vec3f& operator[](rx_size _index);

  union {
    struct {
      vec x, y, z;
    };
    vec a[3];
  };

private:
  static constexpr vec3<T> reduce_rotation_angles(const vec3<T>& _rotate);
};

using mat3x3f = mat3x3<float>;

template<typename T>
constexpr mat3x3<T>::mat3x3()
  : x{1, 0, 0}
  , y{0, 1, 0}
  , z{0, 0, 1}
{
}

template<typename T>
constexpr mat3x3<T>::mat3x3(const vec& _x, const vec& _y, const vec& _z)
  : x{_x}
  , y{_y}
  , z{_z}
{
}

template<typename T>
inline T* mat3x3<T>::data() {
  return x.data();
}

template<typename T>
inline const T* mat3x3<T>::data() const {
  return x.data();
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::scale(const vec3<T>& _scale) {
  return {{_scale.x, 0,       0},
          {0,       _scale.y, 0},
          {0,       0,       _scale.z}};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::rotate(const vec3<T>& _rotate) {
  const auto reduce{reduce_rotation_angles(_rotate)};
  const auto sx{sin(deg_to_rad(-reduce.x))};
  const auto cx{cos(deg_to_rad(-reduce.x))};
  const auto sy{sin(deg_to_rad(-reduce.y))};
  const auto cy{cos(deg_to_rad(-reduce.y))};
  const auto sz{sin(deg_to_rad(-reduce.z))};
  const auto cz{cos(deg_to_rad(-reduce.z))};
  return {{ cy*cz,              cy*-sz,              sy   },
          {-sx*-sy*cz + cx*sz, -sx*-sy*-sz + cx*cz, -sx*cy},
          { cx*-sy*cz + sx*sz,  cx*-sy*-sz + sx*cz,  cx*cy}};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::translate(const vec3<T>& _translate) {
  return {{1, 0, 0},
          {0, 1, 0},
          _translate};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::transpose(const mat3x3& _mat) {
  return {{_mat.x.x, _mat.y.x, _mat.z.x},
          {_mat.x.y, _mat.y.y, _mat.z.y},
          {_mat.x.z, _mat.y.z, _mat.z.z}};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::operator*(const mat3x3& _mat) const {
  return {_mat.x*x.x + _mat.y*x.y + _mat.z*x.z,
          _mat.x*y.x + _mat.y*y.y + _mat.z*y.z,
          _mat.x*z.x + _mat.y*z.y + _mat.z*z.z};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::operator+(const mat3x3& _mat) const {
  return {x + _mat.x, y + _mat.y, z + _mat.z};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar};
}

template<typename T>
inline constexpr mat3x3<T>& mat3x3<T>::operator*=(const mat3x3& _mat) {
  return *this = *this * _mat;
}

template<typename T>
inline constexpr mat3x3<T>& mat3x3<T>::operator+=(const mat3x3& _mat) {
  return *this = *this + _mat;
}

template<typename T>
inline constexpr mat3x3<T>& mat3x3<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr mat3x3<T>& mat3x3<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename T>
inline const vec3f& mat3x3<T>::operator[](rx_size _index) const {
  RX_ASSERT(_index < 3, "out of bounds");
  return a[_index];
}

template<typename T>
inline vec3f& mat3x3<T>::operator[](rx_size _index) {
  RX_ASSERT(_index < 3, "out of bounds");
  return a[_index];
}

template<typename T>
inline constexpr vec3<T> mat3x3<T>::reduce_rotation_angles(const vec3<T>& _rotate) {
  return _rotate.map([](T _angle) {
    while (_angle >  180) {
      _angle -= 360;
    }

    while (_angle < -180) {
      _angle += 360;
    }

    return _angle;
  });
}

} // namespace rx::math

#endif // RX_MATH_MAT3X3_H
