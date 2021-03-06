#ifndef RX_MATH_TRANSFORM_H
#define RX_MATH_TRANSFORM_H

#include "rx/math/mat3x3.h" // Mat3x3f, Vec3f
#include "rx/math/mat4x4.h" // Mat4x4f
#include "rx/math/quat.h"   // Quatf

namespace Rx::Math {

struct Transform {
  constexpr Transform();
  constexpr Transform(Transform* parent);

  Mat4x4f as_mat4() const;
  Mat3x3f as_mat3() const;

  Mat4x4f as_local_mat4() const;
  Mat3x3f as_local_mat3() const;

  Vec3f scale;
  Quatf rotation;
  Vec3f translate;

  const Transform* parent;
};

inline constexpr Transform::Transform()
  : Transform{nullptr}
{
}

inline constexpr Transform::Transform(Transform* _parent)
  : scale{1.0f, 1.0f, 1.0f}
  , translate{}
  , parent{_parent}
{
}

inline Mat4x4f Transform::as_mat4() const {
  const auto& local = as_local_mat4();
  return parent ? local * parent->as_mat4() : local;
}

inline Mat4x4f Transform::as_local_mat4() const {
  return Mat4x4f::scale(scale) * Mat4x4f::rotate(rotation) * Mat4x4f::translate(translate);
}

inline Mat3x3f Transform::as_mat3() const {
  const auto& local = as_local_mat3();
  return parent ? local * parent->as_mat3() : local;
}

inline Mat3x3f Transform::as_local_mat3() const {
  return Mat3x3f::scale(scale) * Mat3x3f::rotate(rotation) * Mat3x3f::translate(translate);
}

} // namespace Rx::Math

#endif // RX_MATH_TRANSFORM_H
