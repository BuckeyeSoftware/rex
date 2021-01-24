#include "rx/math/mat3x3.h"
#include "rx/math/quat.h"

namespace Rx::Math {

template<typename T>
static Mat3x3<T> quat_to_mat3x3(const Quat<T>& q) {
  T x = q.x, y = q.y, z = q.z, w = q.w,
        tx = 2*x, ty = 2*y, tz = 2*z,
        txx = tx*x, tyy = ty*y, tzz = tz*z,
        txy = tx*y, txz = tx*z, tyz = ty*z,
        twx = w*tx, twy = w*ty, twz = w*tz;
  return {{1 - (tyy + tzz), txy - twz, txz + twy},
          {txy + twz, 1 - (txx + tzz), tyz - twx},
          {txz - twy, tyz + twx, 1 - (txx + tyy)}};
}

template<typename T>
Mat3x3<T>::Mat3x3(const Quat<T>& _rotation)
  : Mat3x3<T>{quat_to_mat3x3(_rotation)}
{
}

template<typename T>
Mat3x3<T>::Mat3x3(const Vec3<T>& _scale, const Quat<T>& _rotation)
  : Mat3x3<T>{_rotation}
{
  x *= _scale;
  y *= _scale;
  z *= _scale;
}

template Mat3x3<Float32>::Mat3x3(const Quat<Float32>& _rotation);
template Mat3x3<Float32>::Mat3x3(const Vec3<Float32>& _scale, const Quat<Float32>& _rotation);

} // namespace Rx::Math
