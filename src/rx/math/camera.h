#ifndef RX_MATH_CAMERA_H
#define RX_MATH_CAMERA_H
#include "rx/math/transform.h"

namespace Rx::Math {

struct Camera
  : Transform
{
  constexpr Camera(Transform* _parent = nullptr);

  Mat4x4f view() const;
  Mat4x4f projection;
};

inline constexpr Camera::Camera(Transform* _parent)
  : Transform{_parent}
{
}

inline Mat4x4f Camera::view() const {
  return invert(as_mat4());
}

} // namespace Rx::Math

#endif // RX_MATH_CAMERA_H
