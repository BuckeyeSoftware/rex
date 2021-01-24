#include "rx/core/math/sqrt.h"
#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"

#include "rx/math/quat.h"
#include "rx/math/mat3x3.h"
#include "rx/math/mat3x4.h"

namespace Rx::Math {

template<typename T, template<class> typename M>
static Quat<T> matrix_to_quat(const M<T>& _mat) {
  const T trace = _mat.x.x + _mat.y.y + _mat.z.z;

  if (trace > T(0.0)) {
    const T r = sqrt(T(1.0) + trace);
    const T i = T(0.5) / r;

    return {
      (_mat.z.y - _mat.y.z) * i,
      (_mat.x.z - _mat.z.x) * i,
      (_mat.y.x - _mat.x.y) * i,
      T(0.5) * r
    };
  } else if (_mat.x.x > _mat.y.y && _mat.x.x > _mat.z.z) {
    const T r = sqrt(T(1.0) + _mat.x.x - _mat.y.y - _mat.z.z);
    const T i = T(0.5) / r;

    return {
      T(0.5) * r,
      (_mat.y.x + _mat.x.y) * i,
      (_mat.x.z + _mat.z.x) * i,
      (_mat.z.y - _mat.y.z) * i
    };
  } else if (_mat.y.y > _mat.z.z) {
    const T r = sqrt(T(1.0) + _mat.y.y - _mat.x.x - _mat.z.z);
    const T i = T(0.5) / r;

    return {
      (_mat.y.x + _mat.x.y) * i,
      T(0.5) * r,
      (_mat.z.y + _mat.y.z) * i,
      (_mat.x.z - _mat.z.x) * i
    };
  }

  const T r = sqrt(1.0 + _mat.z.z - _mat.x.x - _mat.y.y);
  const T i = 0.5 / r;

  return {
    (_mat.x.z + _mat.z.x) * i,
    (_mat.z.y + _mat.y.z) * i,
    T(0.5) * r,
    (_mat.y.x - _mat.x.y) * i
  };
}

template<typename T>
Quat<T>::Quat(const Mat3x3<T>& _mat)
  : Quat{matrix_to_quat(_mat)}
{
}

template<typename T>
Quat<T>::Quat(const Mat3x4<T>& _mat)
  : Quat{matrix_to_quat(_mat)}
{
}

template<typename T>
Quat<T>::Quat(const Vec3<T>& _axis, T _angle) {
  const T s = sin(T(0.5) * _angle);
  const T c = cos(T(0.5) * _angle);
  x = s * _axis.x;
  y = s * _axis.y;
  z = s * _axis.z;
  w = c;
}

template Quat<Float32>::Quat(const Mat3x3<Float32>& _mat);
template Quat<Float32>::Quat(const Mat3x4<Float32>& _mat);
template Quat<Float32>::Quat(const Vec3<Float32>& _axis, Float32 _angle);

Float32 length(const Quatf& _value) {
  return sqrt(dot(_value, _value));
}

Quatf normalize(const Quatf& _value) {
  return _value * (1.0f / Math::length(_value));
}

} // namespace Rx::Math
