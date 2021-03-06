#include "rx/math/mat4x4.h"
#include "rx/math/quat.h"
#include "rx/math/range.h"
#include "rx/math/trig.h"
#include "rx/math/compare.h"

#include "rx/core/math/sin.h" // sin
#include "rx/core/math/cos.h" // cos
#include "rx/core/math/tan.h" // tan

namespace Rx::Math {

template<typename T>
constexpr Vec3<T> reduce_rotation_angles(const Vec3<T>& _rotate) {
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

template<typename T>
constexpr T det2x2(T a, T b, T c, T d) {
  return a*d - b*c;
}

template<typename T>
constexpr T det3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3) {
  return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}

template<typename T>
Mat4x4<T> Mat4x4<T>::rotate(const Quat<T>& _quat) {
  T x = _quat.x, y = _quat.y, z = _quat.z, w = _quat.w,
        tx = 2*x, ty = 2*y, tz = 2*z,
        txx = tx*x, tyy = ty*y, tzz = tz*z,
        txy = tx*y, txz = tx*z, tyz = ty*z,
        twx = w*tx, twy = w*ty, twz = w*tz;
  return {
    {1 - (tyy + tzz), txy - twz, txz + twy, 0},
    {txy + twz, 1 - (txx + tzz), tyz - twx, 0},
    {txz - twy, tyz + twx, 1 - (txx + tyy), 0},
    {0, 0, 0, 1}
  };
}

template<typename T>
Mat4x4<T> Mat4x4<T>::rotate(const Vec3<T>& _rotate) {
  const auto reduce = reduce_rotation_angles(_rotate);
  const auto sx = sin(deg_to_rad(-reduce.x));
  const auto cx = cos(deg_to_rad(-reduce.x));
  const auto sy = sin(deg_to_rad(-reduce.y));
  const auto cy = cos(deg_to_rad(-reduce.y));
  const auto sz = sin(deg_to_rad(-reduce.z));
  const auto cz = cos(deg_to_rad(-reduce.z));
  return {{ cy*cz,              cy*-sz,              sy,    0},
          {-sx*-sy*cz + cx*sz, -sx*-sy*-sz + cx*cz, -sx*cy, 0},
          { cx*-sy*cz + sx*sz,  cx*-sy*-sz + sx*cz,  cx*cy, 0},
          { 0,                  0,                   0,     1}};
}

template<typename T>
Mat4x4<T> invert(const Mat4x4<T>& _mat) {
  const auto a1{_mat.x.x}, a2{_mat.x.y}, a3{_mat.x.z}, a4{_mat.x.w};
  const auto b1{_mat.y.x}, b2{_mat.y.y}, b3{_mat.y.z}, b4{_mat.y.w};
  const auto c1{_mat.z.x}, c2{_mat.z.y}, c3{_mat.z.z}, c4{_mat.z.w};
  const auto d1{_mat.w.x}, d2{_mat.w.y}, d3{_mat.w.z}, d4{_mat.w.w};

  const auto det1 =  det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
  const auto det2 = -det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
  const auto det3 =  det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
  const auto det4 = -det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

  const auto det = a1*det1 + b1*det2 + c1*det3 + d1*det4;

  if (epsilon_compare(det, 0.0f)) {
    return {};
  }

  const auto invdet = T{1} / det;

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
Mat4x4<T> perspective(T _fov, const Range<T>& _planes, T _aspect) {
  const T range = _planes.min - _planes.max;
  const T half = tan(deg_to_rad(_fov*T{.5}));

  if (_aspect < 1) {
    return {{1 / half,             0,                    0,                                      0},
            {0,                    1 / (half / _aspect), 0,                                      0},
            {0,                    0,                    -(_planes.min + _planes.max) / range,   1},
            {0,                    0,                    2 * _planes.max * _planes.min / range,  0}};
  } else {
    return {{1 / (half * _aspect), 0,                    0,                                      0},
            {0,                    1 / half,             0,                                      0},
            {0,                    0,                    -(_planes.min + _planes.max) / range,   1},
            {0,                    0,                    2 * _planes.max * _planes.min / range,  0}};
  }
}

template Mat4x4<Float32> Mat4x4<Float32>::rotate(const Vec3<Float32>& _rotate);
template Mat4x4<Float32> Mat4x4<Float32>::rotate(const Quat<Float32>& _rotate);

template Mat4x4<Float32> perspective(Float32 _fov, const Range<Float32>& _planes, Float32 _aspect);
template Mat4x4<Float32> invert(const Mat4x4<Float32>& _mat);

} // namespace Rx::Math