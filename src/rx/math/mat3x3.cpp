#include "rx/math/mat3x3.h"
#include "rx/math/quat.h"

#include "rx/core/math/cos.h" // cos
#include "rx/core/math/sin.h" // sin

#include "rx/math/trig.h" // deg_to_rad

namespace Rx::Math {

template<typename T>
constexpr Vec3<T> reduce_rotation_angles(const Vec3<T>& _rotate) {
  return _rotate.map([](T _angle) {
    while (_angle > 180) {
      _angle -= 360;
    }

    while (_angle < -180) {
      _angle += 360;
    }

    return _angle;
  });
}

template<typename T>
Mat3x3<T> Mat3x3<T>::rotate(const Quat<T>& _quat) {
  T x = _quat.x, y = _quat.y, z = _quat.z, w = _quat.w,
        tx = 2*x, ty = 2*y, tz = 2*z,
        txx = tx*x, tyy = ty*y, tzz = tz*z,
        txy = tx*y, txz = tx*z, tyz = ty*z,
        twx = w*tx, twy = w*ty, twz = w*tz;
  return {{1 - (tyy + tzz), txy - twz, txz + twy},
          {txy + twz, 1 - (txx + tzz), tyz - twx},
          {txz - twy, tyz + twx, 1 - (txx + tyy)}};
}

template<typename T>
Mat3x3<T> Mat3x3<T>::rotate(const Vec3<T>& _rotate) {
  const auto reduce = reduce_rotation_angles(_rotate);
  const auto sx = sin(deg_to_rad(-reduce.x));
  const auto cx = cos(deg_to_rad(-reduce.x));
  const auto sy = sin(deg_to_rad(-reduce.y));
  const auto cy = cos(deg_to_rad(-reduce.y));
  const auto sz = sin(deg_to_rad(-reduce.z));
  const auto cz = cos(deg_to_rad(-reduce.z));
  return {{ cy*cz,              cy*-sz,              sy   },
          {-sx*-sy*cz + cx*sz, -sx*-sy*-sz + cx*cz, -sx*cy},
          { cx*-sy*cz + sx*sz,  cx*-sy*-sz + sx*cz,  cx*cy}};
}

template<typename T>
Mat3x3<T> Mat3x3<T>::rotate(const Quat<T>& _quat, const Vec3<T>& _scale) {
  auto result = rotate(_quat);
  result.x *= _scale;
  result.y *= _scale;
  result.z *= _scale;
  return result;
}

template Mat3x3<Float32> Mat3x3<Float32>::rotate(const Vec3<Float32>&);
template Mat3x3<Float32> Mat3x3<Float32>::rotate(const Quat<Float32>&);
template Mat3x3<Float32> Mat3x3<Float32>::rotate(const Quat<Float32>& _quat, const Vec3<Float32>& _scale);

} // namespace Rx::Math
