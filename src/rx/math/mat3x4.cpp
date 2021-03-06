#include "rx/math/mat3x4.h"
#include "rx/math/mat3x3.h"
#include "rx/math/quat.h"

namespace Rx::Math {

template<typename T>
Mat3x4<T> invert(const Mat3x4<T>& _mat) {
  Vec3<T> inverse_rotation_x{_mat.x.x, _mat.y.x, _mat.z.x};
  Vec3<T> inverse_rotation_y{_mat.x.y, _mat.y.y, _mat.z.y};
  Vec3<T> inverse_rotation_z{_mat.x.z, _mat.y.z, _mat.z.z};

  inverse_rotation_x /= dot(inverse_rotation_x, inverse_rotation_x);
  inverse_rotation_y /= dot(inverse_rotation_y, inverse_rotation_y);
  inverse_rotation_z /= dot(inverse_rotation_z, inverse_rotation_z);

  const Vec3<T> translate{_mat.x.w, _mat.y.w, _mat.z.w};

  return {{inverse_rotation_x.x, inverse_rotation_x.y, inverse_rotation_x.z, -dot(inverse_rotation_x, translate)},
          {inverse_rotation_y.x, inverse_rotation_y.y, inverse_rotation_y.z, -dot(inverse_rotation_y, translate)},
          {inverse_rotation_z.x, inverse_rotation_z.y, inverse_rotation_z.z, -dot(inverse_rotation_z, translate)}};
}

template<typename T>
Mat3x4<T>::Mat3x4(const Mat3x3<T>& _scale_rotate, const Vec3<T>& _translation)
  : x{_scale_rotate.x.x, _scale_rotate.x.y, _scale_rotate.x.z, _translation.x}
  , y{_scale_rotate.y.x, _scale_rotate.y.y, _scale_rotate.y.z, _translation.y}
  , z{_scale_rotate.z.x, _scale_rotate.z.y, _scale_rotate.z.z, _translation.z}
{
}

template<typename T>
Mat3x4<T>::Mat3x4(const Quat<T>& _rotation, const Vec3<T>& _translation)
  : Mat3x4{Mat3x3<T>::rotate(_rotation), _translation}
{
}

template<typename T>
Mat3x4<T>::Mat3x4(const Vec3<T>& _scale, const Quat<T>& _rotation,
                  const Vec3<T>& _translation)
  : Mat3x4{Mat3x3<T>::rotate(_rotation, _scale), _translation}
{
}

template Mat3x4<Float32>::Mat3x4(const Mat3x3<Float32>& _rotation,
                                const Vec3<Float32>& _translation);
template Mat3x4<Float32>::Mat3x4(const Quat<Float32>& _rotation,
                                const Vec3<Float32>& _translation);
template Mat3x4<Float32>::Mat3x4(const Vec3<Float32>& _scale,
                                const Quat<Float32>& _rotation, const Vec3<Float32>& _translation);


template Mat3x4<Float32> invert(const Mat3x4<Float32>& _mat);

} // namespace Rx::Math
