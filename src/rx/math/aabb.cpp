#include "rx/math/aabb.h"
#include "rx/math/mat4x4.h"
#include "rx/math/ray.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Math {

void AABB::expand(const Vec3f& _point) {
  m_min.x = Algorithm::min(_point.x, m_min.x);
  m_min.y = Algorithm::min(_point.y, m_min.y);
  m_min.z = Algorithm::min(_point.z, m_min.z);

  m_max.x = Algorithm::max(_point.x, m_max.x);
  m_max.y = Algorithm::max(_point.y, m_max.y);
  m_max.z = Algorithm::max(_point.z, m_max.z);
}

AABB AABB::transform(const Mat4x4f& _mat) const {
  const Vec3f& x{_mat.x.x, _mat.x.y, _mat.x.z};
  const Vec3f& y{_mat.y.x, _mat.y.y, _mat.y.z};
  const Vec3f& z{_mat.z.x, _mat.z.y, _mat.z.z};
  const Vec3f& w{_mat.w.x, _mat.w.y, _mat.w.z};

  const Vec3f& xa{x * m_min.x};
  const Vec3f& xb{x * m_max.x};
  const Vec3f& ya{y * m_min.y};
  const Vec3f& yb{y * m_max.y};
  const Vec3f& za{z * m_min.z};
  const Vec3f& zb{z * m_max.z};

  return {
    Math::min(xa, xb) + Math::min(ya, yb) + Math::min(za, zb) + w,
    Math::max(xa, xb) + Math::max(ya, yb) + Math::max(za, zb) + w
  };
}

Optional<Vec3f> AABB::ray_intersect(const Ray& _ray) const {
  const auto& origin = _ray.point();
  const auto& dir = _ray.direction();

  const auto& inv_dir = 1.0f / dir;

  const auto t1 = (m_min.x - origin.x) * inv_dir.x;
  const auto t2 = (m_max.x - origin.x) * inv_dir.x;
  const auto t3 = (m_min.y - origin.y) * inv_dir.y;
  const auto t4 = (m_max.y - origin.y) * inv_dir.y;
  const auto t5 = (m_min.z - origin.z) * inv_dir.z;
  const auto t6 = (m_max.z - origin.z) * inv_dir.z;

  const auto tmin = Algorithm::max(
    Algorithm::min(t1, t2),
    Algorithm::min(t3, t4),
    Algorithm::min(t5, t6)
  );

  const auto tmax = Algorithm::min(
    Algorithm::max(t1, t2),
    Algorithm::max(t3, t4),
    Algorithm::max(t5, t6)
  );

  if (tmax < 0.0f || tmin > tmax) {
    return nullopt;
  }

  return tmin < 0.0f ? origin : _ray.point_at_time(tmin);
}

} // namespace Rx::Math
