#ifndef RX_MATH_MAT3X3_H
#define RX_MATH_MAT3X3_H

#include <rx/math/vec3.h> // vec3
#include <rx/math/trig.h> // deg_to_rad, sin, cos

namespace rx::math {

template<typename T>
struct mat3x3 {
  using vec = vec3<T>;

  constexpr mat3x3();
  constexpr mat3x3(const vec& x, const vec& y, const vec& z);

  static constexpr mat3x3 scale(const vec3<T>& scale);
  static constexpr mat3x3 rotate(const vec3<T>& rotate);
  static constexpr mat3x3 translate(const vec3<T>& translate);
  static constexpr mat3x3 transpose(const mat3x3& mat);

  const T* data() const;

  vec x, y, z;
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
constexpr mat3x3<T>::mat3x3(const vec& x, const vec& y, const vec& z)
  : x{x}
  , y{y}
  , z{z}
{
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::scale(const vec3<T>& scale) {
  return {{scale.x, 0,       0},
          {0,       scale.y, 0},
          {0,       0,       scale.z}};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::rotate(const vec3<T>& rotate) {
  const auto sx{sin(deg_to_rad(-rotate.x))};
  const auto cx{cos(deg_to_rad(-rotate.x))};
  const auto sy{sin(deg_to_rad(-rotate.y))};
  const auto cy{cos(deg_to_rad(-rotate.y))};
  const auto sz{sin(deg_to_rad(-rotate.z))};
  const auto cz{cos(deg_to_rad(-rotate.z))};
  return {{ cy*cz,              cy*-sz,              sy},
          {-sx*-sy*cz + cx*sz, -sx*-sy*-sz + cx*cz, -sx*cy},
          { cx*-sy*cz + sx*sz,  cx*-sy*-sz + sx*cz,  cx*cy}};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::translate(const vec3<T>& translate) {
  return {{1, 0, 0},
          {0, 1, 0},
          translate};
}

template<typename T>
inline constexpr mat3x3<T> mat3x3<T>::transpose(const mat3x3& mat) {
  return {{mat.x.x, mat.y.x, mat.z.x},
          {mat.x.y, mat.y.y, mat.z.y},
          {mat.x.z, mat.y.z, mat.z.z}};
}

template<typename T>
inline constexpr mat3x3<T> operator*(const mat3x3<T>& a, const mat3x3<T>& b) {
  return {b.x*a.x.x + b.y*a.x.y + b.z*a.x.z,
          b.x*a.y.x + b.y*a.y.y + b.z*a.y.z,
          b.x*a.z.x + b.y*a.z.y + b.z*a.z.z};
}

template<typename T>
inline const T* mat3x3<T>::data() const {
  // NOTE: this only works because mat3x3 is contiguous in memory
  return x.data();
}

} // namespace rx::math

#endif // RX_MATH_MAT3X3_H
