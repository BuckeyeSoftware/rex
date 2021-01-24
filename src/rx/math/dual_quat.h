#ifndef RX_MATH_DUAL_QUAT_H
#define RX_MATH_DUAL_QUAT_H
#include "rx/math/quat.h"
#include "rx/math/vec3.h"

namespace Rx::Math {

template<typename T>
struct Mat3x4;

template<typename T>
struct DualQuat {
  Quat<T> real;
  Quat<T> dual;

  // Construct from a rotation quaternion and translation.
  constexpr DualQuat();
  constexpr DualQuat(const Quat<T>& _rotation, const Vec3<T>& _translation);
  constexpr DualQuat(const Quat<T>& _rotation, const Quat<T>& _dual);
  DualQuat(const Mat3x4<T>& _transform);

  constexpr DualQuat operator+(const DualQuat& _other) const;
  constexpr DualQuat operator*(T _scalar) const;

  DualQuat lerp(const DualQuat& _to, Float32 _t) const {
    // Handle antipodal.
    const auto k = dot(real, _to.real) < 0 ? -_t : _t;
    auto reall = real * (1.0f - _t) + _to.real * k;
    auto duall = dual * (1.0f - _t) + _to.dual * k;
    return {reall, duall};
  }
};

template<typename T>
constexpr DualQuat<T>::DualQuat()
  : real{0, 0, 0, 1}
  , dual{0, 0, 0, 0}
{
}

template<typename T>
constexpr DualQuat<T>::DualQuat(const Quat<T>& _rotation, const Vec3<T>& _t)
  : real{_rotation}
  , dual{T( 0.5) * ( _t.x * real.w + _t.y * real.z - _t.z * real.y),
         T( 0.5) * (-_t.x * real.z + _t.y * real.w + _t.z * real.x),
         T( 0.5) * ( _t.x * real.y - _t.y * real.x + _t.z * real.w),
         T(-0.5) * ( _t.x * real.x + _t.y * real.y + _t.z * real.z)}
{
}

template<typename T>
constexpr DualQuat<T>::DualQuat(const Quat<T>& _rotation, const Quat<T>& _dual)
  : real{_rotation}
  , dual{_dual}
{
}

template<typename T>
constexpr DualQuat<T> DualQuat<T>::operator+(const DualQuat& _other) const {
  return {real + _other.real, dual + _other.dual};
}

template<typename T>
constexpr DualQuat<T> DualQuat<T>::operator*(T _scalar) const {
  return {real * _scalar, dual * _scalar};
}

using DualQuatf = DualQuat<Float32>;

DualQuatf normalize(const DualQuatf& _dquat);

} // namespace Rx::Math

#endif // RX_MATH_DUAL_QUAT_H
