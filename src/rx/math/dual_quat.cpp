#include "rx/math/dual_quat.h"
#include "rx/math/mat3x4.h"

#include "rx/core/math/cos.h"
#include "rx/core/math/sin.h"

namespace Rx::Math {

template<typename T>
DualQuat<T>::DualQuat(const Mat3x4<T>& _transform)
  : DualQuat{Quat<T>{_transform}, {_transform.x.w, _transform.y.w, _transform.z.w}}
{
}

template DualQuat<Float32>::DualQuat(const Mat3x4f& _transform);

} // namespace Rx::Math
