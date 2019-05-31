#ifndef RX_MATH_MAT4X4_H
#define RX_MATH_MAT4X4_H

#include "rx/math/vec3.h" // vec3
#include "rx/math/vec4.h" // vec4
#include "rx/math/trig.h" // deg_to_rad, sin, cos, tan

namespace rx::math {

template<typename T>
struct mat4x4 {
  using vec = vec4<T>;

  constexpr mat4x4();
  constexpr mat4x4(const vec& _x, const vec& _y, const vec& _z, const vec& _w);

  const T* data() const;

  static constexpr mat4x4 scale(const vec3<T>& _scale);
  static constexpr mat4x4 rotate(const vec3<T>& _rotate);
  static constexpr mat4x4 translate(const vec3<T>& _translate);
  static constexpr mat4x4 transpose(const mat4x4& _mat);
  static constexpr mat4x4 invert(const mat4x4& _mat);
  static constexpr mat4x4 perspective(T _fov, const range<T>& _planes, T _aspect);

  constexpr mat4x4 operator*(const mat4x4& _mat) const;
  constexpr mat4x4 operator+(const mat4x4& _mat) const;

  constexpr mat4x4 operator*(T _scalar) const;
  constexpr mat4x4 operator+(T _scalar) const;

  constexpr mat4x4& operator*=(const mat4x4& _mat);
  constexpr mat4x4& operator+=(const mat4x4& _mat);

  constexpr mat4x4& operator*=(T _scalar);
  constexpr mat4x4& operator+=(T _scalar);

  vec x, y, z, w;

private:
  static constexpr T det2x2(T a, T b, T c, T d);
  static constexpr T det3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3);
};

using mat4x4f = mat4x4<float>;

template<typename T>
constexpr mat4x4<T>::mat4x4()
  : x{1, 0, 0, 0}
  , y{0, 1, 0, 0}
  , z{0, 0, 1, 0}
  , w{0, 0, 0, 1}
{
}

template<typename T>
constexpr mat4x4<T>::mat4x4(const vec& x, const vec& y, const vec& z, const vec& w)
  : x{x}
  , y{y}
  , z{z}
  , w{w}
{
}

template<typename T>
inline const T* mat4x4<T>::data() const {
  return x.data();
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::scale(const vec3<T>& scale) {
  return {{scale.x, 0, 0, 0},
          {0, scale.y, 0, 0},
          {0, 0, scale.z, 0},
          {0, 0, 0, 1}};
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::rotate(const vec3<T>& rotate) {
  const auto sx{sin(deg_to_rad(-rotate.x))};
  const auto cx{cos(deg_to_rad(-rotate.x))};
  const auto sy{sin(deg_to_rad(-rotate.y))};
  const auto cy{cos(deg_to_rad(-rotate.y))};
  const auto sz{sin(deg_to_rad(-rotate.z))};
  const auto cz{cos(deg_to_rad(-rotate.z))};
  return {{ cy*cz,              cy*-sz,              sy,    0},
          {-sx*-sy*cz + cx*sz, -sx*-sy*-sz + cx*cz, -sx*cy, 0},
          { cx*-sy*cz + sx*sz,  cx*-sy*-sz + sx*cz,  cx*cy, 0},
          { 0,                  0,                   0,     1}};
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::translate(const vec3<T>& translate) {
  return {{1,           0,           0,           0},
          {0,           1,           0,           0},
          {0,           0,           1,           0},
          {translate.x, translate.y, translate.z, 1}};

}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::transpose(const mat4x4& mat) {
  return {{mat.x.x, mat.y.x, mat.z.x, mat.w.x},
          {mat.x.y, mat.y.y, mat.z.y, mat.w.y},
          {mat.x.z, mat.y.z, mat.z.z, mat.w.z}};
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::invert(const mat4x4& mat) {
  const auto a1{mat.x.x}, a2{mat.x.y}, a3{mat.x.z}, a4{mat.x.w};
  const auto b1{mat.y.x}, b2{mat.y.y}, b3{mat.y.z}, b4{mat.y.w};
  const auto c1{mat.z.x}, c2{mat.z.y}, c3{mat.z.z}, c4{mat.z.w};
  const auto d1{mat.w.x}, d2{mat.w.y}, d3{mat.w.z}, d4{mat.w.w};

  const auto det1{ det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4)};
  const auto det2{-det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4)};
  const auto det3{ det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4)};
  const auto det4{-det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4)};

  const auto det{a1*det1 + b1*det2 + c1*det3 + d1*det4};
  const auto invdet{T{1} / det};

  return {
    {
      det1 * invdet,
      det2 * invdet,
      det3 * invdet,
      det4 * invdet
    },{
      -det3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4) * invdet,
       det3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4) * invdet,
      -det3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4) * invdet,
       det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4) * invdet,
    },{
       det3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4) * invdet,
      -det3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4) * invdet,
       det3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4) * invdet,
      -det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4) * invdet
    },{
      -det3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3) * invdet,
       det3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3) * invdet,
      -det3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3) * invdet,
       det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3) * invdet
    }
  };
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::perspective(T _fov, const range<T>& _planes, T _aspect) {
  const T range{_planes.min - _planes.max};
  const T half{tan(deg_to_rad(_fov*T{.5}))};
  if (_aspect < T{1}) {
    return {{T{1} / half,             T{0},                    T{0},                                      T{0}},
            {T{0},                    T{1} / (half / _aspect), T{0},                                      T{0}},
            {T{0},                    T{0},                    -(_planes.min + _planes.max) / range,      T{1}},
            {T{0},                    T{0},                    T{2} * _planes.max * _planes.min / range,  T{0}}};
  } else {
    return {{T{1} / (half * _aspect), T{0},                    T{0},                                      T{0}},
            {T{0},                    T{1} / half,             T{0},                                      T{0}},
            {T{0},                    T{0},                    -(_planes.min + _planes.max) / range,      T{1}},
            {T{0},                    T{0},                    T{2} * _planes.max * _planes.min / range,  T{0}}};
  }
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::operator*(const mat4x4& _mat) const {
  return {_mat.x*x.x + _mat.y*x.y + _mat.z*x.z + _mat.w*x.w,
          _mat.x*y.x + _mat.y*y.y + _mat.z*y.z + _mat.w*y.w,
          _mat.x*z.x + _mat.y*z.y + _mat.z*z.z + _mat.w*z.w,
          _mat.x*w.x + _mat.y*w.y + _mat.z*w.z + _mat.w*w.w};
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::operator+(const mat4x4& _mat) const {
  return {x + _mat.x, y + _mat.y, z + _mat.z, w + _mat.w};
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::operator*(T _scalar) const {
  return {x * _scalar, y * _scalar, z * _scalar, w * _scalar};
}

template<typename T>
inline constexpr mat4x4<T> mat4x4<T>::operator+(T _scalar) const {
  return {x + _scalar, y + _scalar, z + _scalar, w + _scalar};
}

template<typename T>
inline constexpr mat4x4<T>& mat4x4<T>::operator*=(const mat4x4& _mat) {
  return *this = *this * _mat;
}

template<typename T>
inline constexpr mat4x4<T>& mat4x4<T>::operator+=(const mat4x4& _mat) {
  return *this = *this + _mat;
}

template<typename T>
inline constexpr mat4x4<T>& mat4x4<T>::operator*=(T _scalar) {
  return *this = *this * _scalar;
}

template<typename T>
inline constexpr mat4x4<T>& mat4x4<T>::operator+=(T _scalar) {
  return *this = *this + _scalar;
}

template<typename T>
inline constexpr T mat4x4<T>::det2x2(T a, T b, T c, T d) {
  return a*d - b*c;
}

template<typename T>
inline constexpr T mat4x4<T>::det3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3) {
  return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}

} // namespace rx::math

#endif // RX_MATH_MAT4X4_H
