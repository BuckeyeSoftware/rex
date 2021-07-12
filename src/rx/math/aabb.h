#ifndef RX_MATH_AABB_H
#define RX_MATH_AABB_H
#include "rx/math/vec3.h"
#include "rx/core/optional.h"

namespace Rx::Math {

template<typename T>
struct Mat4x4;

using Mat4x4f = Mat4x4<Float32>;

struct Ray;

struct AABB {
  constexpr AABB();
  constexpr AABB(const Vec3f& _min, const Vec3f& _max);

  void expand(const Vec3f& _point);
  void expand(const AABB& _bounds);

  void reset();

  AABB transform(const Mat4x4f& _mat) const;

  const Vec3f& min() const &;
  const Vec3f& max() const &;

  Vec3f origin() const;
  Vec3f scale() const;

  bool is_point_inside(const Vec3f& _point) const;

  Optional<Vec3f> ray_intersect(const Ray& _ray) const;

private:
  Vec3f m_min;
  Vec3f m_max;
};

inline constexpr AABB::AABB()
  : m_min{ FLT_MAX,  FLT_MAX,  FLT_MAX}
  , m_max{-FLT_MAX, -FLT_MAX, -FLT_MAX}
{
}

inline constexpr AABB::AABB(const Vec3f& _min, const Vec3f& _max)
  : m_min{_min}
  , m_max{_max}
{
}

inline void AABB::expand(const AABB& _bounds) {
  expand(_bounds.m_min);
  expand(_bounds.m_max);
}

inline void AABB::reset() {
  m_min = { FLT_MAX, FLT_MAX, FLT_MAX };
  m_max = -m_min;
}

inline const Vec3f& AABB::min() const & {
  return m_min;
}

inline const Vec3f& AABB::max() const & {
  return m_max;
}

inline Vec3f AABB::origin() const {
  return (m_min + m_max) * 0.5f;
}

inline Vec3f AABB::scale() const {
  return (m_max - m_min) * 0.5f;
}

inline bool AABB::is_point_inside(const Vec3f& _point) const {
  return _point.x > m_min.x
      && _point.y > m_min.y
      && _point.z > m_min.z
      && _point.x < m_max.x
      && _point.y < m_max.y
      && _point.z < m_max.z;
}

}

#endif // RX_MATH_AABB_H
