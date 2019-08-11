#ifndef RX_MATH_CAMERA_H
#define RX_MATH_CAMERA_H
#include "rx/math/transform.h"

namespace rx::math {

//! camera
struct camera : transform {
  //! construct with projection matrix and optional parent transform
  constexpr camera(const mat4x4f& _projection, transform* _parent = nullptr);

  mat4x4f view() const;
  mat4x4f projection() const;

private:
  mat4x4f m_projection;
};

inline constexpr camera::camera(const mat4x4f& _projection, transform* _parent)
  : transform{_parent}
  , m_projection{_projection}
{
}

mat4x4f camera::view() const {
  return mat4x4f::invert(to_mat4());
}

mat4x4f camera::projection() const {
  return m_projection;
}

} // namespace rx::math

#endif // RX_MATH_CAMERA_H