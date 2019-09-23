#ifndef RX_MATH_FRUSTUM_H
#define RX_MATH_FRUSTUM_H
#include "rx/math/plane.h"
#include "rx/math/mat4x4.h"

namespace rx::math {

struct aabb;
struct frustum {
  frustum(const mat4x4f& _view_projection);

  bool is_aabb_inside(const aabb& _aabb) const;

private:
  plane m_planes[6];
};

} // namespace rx::math

#endif // RX_MATH_FRUSTUM_H
