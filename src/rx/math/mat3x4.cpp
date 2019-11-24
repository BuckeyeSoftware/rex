#include "rx/math/mat3x4.h"
#include "rx/math/mat3x3.h"
#include "rx/math/quat.h"

namespace rx::math {

template<typename T>
mat3x4<T>::mat3x4(const mat3x3<T>& _scale_rotate, const vec3<T>& _translation)
  : x{_scale_rotate.x.x, _scale_rotate.x.y, _scale_rotate.x.z, _translation.x}
  , y{_scale_rotate.y.x, _scale_rotate.y.y, _scale_rotate.y.z, _translation.y}
  , z{_scale_rotate.z.x, _scale_rotate.z.y, _scale_rotate.z.z, _translation.z}
{
}

template<typename T>
mat3x4<T>::mat3x4(const quat<T>& _rotation, const vec3<T>& _translation)
  : mat3x4{mat3x3<T>{_rotation}, _translation}
{
}

template<typename T>
mat3x4<T>::mat3x4(const vec3<T>& _scale, const quat<T>& _rotation,
  const vec3<T>& _translation)
  : mat3x4{mat3x3<T>{ _scale, _rotation}, _translation}
{
}

template mat3x4<rx_f32>::mat3x4(const mat3x3<rx_f32>& _rotation,
  const vec3<rx_f32>& _translation);
template mat3x4<rx_f32>::mat3x4(const quat<rx_f32>& _rotation,
  const vec3<rx_f32>& _translation);
template mat3x4<rx_f32>::mat3x4(const vec3<rx_f32>& _scale,
  const quat<rx_f32>& _rotation, const vec3<rx_f32>& _translation);

} // namespace rx::math
