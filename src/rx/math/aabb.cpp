#include "rx/math/aabb.h"
#include "rx/math/mat4x4.h"

namespace rx::math {

void aabb::expand(const vec3f& _point) {
  m_min.x = algorithm::min(_point.x, m_min.x);
  m_min.y = algorithm::min(_point.y, m_min.y);
  m_min.z = algorithm::min(_point.z, m_min.z);

  m_max.x = algorithm::max(_point.x, m_max.x);
  m_max.y = algorithm::max(_point.y, m_max.y);
  m_max.z = algorithm::max(_point.z, m_max.z);
}

aabb aabb::transform(const mat4x4f& _mat) const {
  const vec3f& x{_mat.x.x, _mat.x.y, _mat.x.z};
  const vec3f& y{_mat.y.x, _mat.y.y, _mat.y.z};
  const vec3f& z{_mat.z.x, _mat.z.y, _mat.z.z};
  const vec3f& w{_mat.w.x, _mat.w.y, _mat.w.z};

  const vec3f& xa{x * m_min.x};
  const vec3f& xb{x * m_max.x};
  const vec3f& ya{y * m_min.y};
  const vec3f& yb{y * m_max.y};
  const vec3f& za{z * m_min.z};
  const vec3f& zb{z * m_max.z};

  const auto min{[](const vec3f& _lhs, const vec3f& _rhs) -> vec3f {
    return {
      algorithm::min(_lhs.x, _rhs.x),
      algorithm::min(_lhs.y, _rhs.y),
      algorithm::min(_lhs.z, _rhs.z)
    };
  }};

  const auto max{[](const vec3f& _lhs, const vec3f& _rhs) -> vec3f {
    return {
      algorithm::max(_lhs.x, _rhs.x),
      algorithm::max(_lhs.y, _rhs.y),
      algorithm::max(_lhs.z, _rhs.z)
    };
  }};

  return {
    min(xa, xb) + min(ya, yb) + min(za, zb) + w,
    max(xa, xb) + max(ya, yb) + max(za, zb) + w
  };
}

} // namespace rx::math
