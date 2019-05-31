#include "rx/math/mat3x3.h"
#include "rx/math/quat.h"

namespace rx::math {

template<typename T>
static mat3x3<T> quat_to_mat3x3(const quat<T>& _quat) {
  return {
    {
      T{1} - T{2}*_quat.y*_quat.y - T{2}*_quat.z*_quat.z,
             T{2}*_quat.x*_quat.y - T{2}*_quat.z*_quat.w,
             T{2}*_quat.x*_quat.z + T{2}*_quat.y*_quat.w
    },{
             T{2}*_quat.x*_quat.y + T{2}*_quat.z*_quat.w,
      T{1} - T{2}*_quat.x*_quat.x - T{2}*_quat.z*_quat.z,
             T{2}*_quat.y*_quat.z - T{2}*_quat.x*_quat.w
    },{
             T{2}*_quat.x*_quat.z - T{2}*_quat.y*_quat.w,
             T{2}*_quat.y*_quat.z + T{2}*_quat.x*_quat.w,
      T{1} - T{2}*_quat.x*_quat.x - T{2}*_quat.y*_quat.y
    }
  };
}

template<typename T>
mat3x3<T>::mat3x3(const quat<T>& _rotation)
  : mat3x3<T>{quat_to_mat3x3(_rotation)}
{
}

template<typename T>
mat3x3<T>::mat3x3(const vec3<T>& _scale, const quat<T>& _rotation)
  : mat3x3<T>{_rotation}
{
  x *= _scale;
  y *= _scale;
  z *= _scale;
}

template mat3x3<rx_f32>::mat3x3(const quat<rx_f32>& _rotation);
template mat3x3<rx_f32>::mat3x3(const vec3<rx_f32>& _scale, const quat<rx_f32>& _rotation);

} // namespace rx::math