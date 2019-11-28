#ifndef RX_MATH_CAMERA_H
#define RX_MATH_CAMERA_H
#include "rx/math/transform.h"

namespace rx::math {

struct camera
  : transform
{
  constexpr camera(transform* _parent = nullptr);
  mat4x4f view() const;
  mat4x4f projection;
};

inline constexpr camera::camera(transform* _parent)
  : transform{_parent}
{
}

inline mat4x4f camera::view() const {
  return mat4x4f::invert(to_mat4());
}

} // namespace rx::math

#endif // RX_MATH_CAMERA_H
