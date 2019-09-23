#include "rx/math/frustum.h"
#include "rx/math/aabb.h"

namespace rx::math {

frustum::frustum(const mat4x4f& _view_projection) {
  const vec4f& x{_view_projection.x};
  const vec4f& y{_view_projection.y};
  const vec4f& z{_view_projection.z};
  const vec4f& w{_view_projection.w};

  m_planes[0] = {{x.w + x.x, y.w + y.x, z.w + z.x}, -(w.w + w.x)};
  m_planes[1] = {{x.w - x.x, y.w - y.x, z.w - z.x}, -(w.w - w.x)};
  m_planes[2] = {{x.w - x.y, y.w - y.y, z.w - z.y}, -(w.w - w.y)};
  m_planes[3] = {{x.w + x.y, y.w + y.y, z.w + z.y}, -(w.w + w.y)};
  m_planes[4] = {{x.w - x.z, y.w - y.z, z.w - z.z}, -(w.w - w.z)};
  m_planes[5] = {{x.w + x.z, y.w + y.z, z.w + z.z}, -(w.w + w.z)};
}

bool frustum::is_aabb_inside(const aabb& _aabb) const {
  const auto& min{_aabb.min()};
  const auto& max{_aabb.max()};
  for (rx_size i{0}; i < 6; i++) {
    const auto& plane{m_planes[i]};
    const auto& normal{plane.normal()};
    const rx_f32 distance{normal.x * (normal.x < 0.0f ? min.x : max.x) +
                          normal.y * (normal.y < 0.0f ? min.y : max.y) +
                          normal.z * (normal.z < 0.0f ? min.z : max.z)};
    if (distance <= plane.distance()) {
      return false;
    }
  }
  return true;
}

} // namespace rx::math
