#ifndef RX_MATH_AABB_H
#define RX_MATH_AABB_H
#include "rx/math/vec3.h"

#include "rx/core/algorithm/min.h"
#include "rx/core/algorithm/max.h"

namespace rx::math {

struct aabb {
  constexpr aabb();

  void expand(const vec3f& _point);
  void expand(const aabb& _bounds);

  const vec3f& min() const &;
  const vec3f& max() const &;

  vec3f origin() const;
  vec3f scale() const;

private:
  vec3f m_min;
  vec3f m_max;
};

inline constexpr aabb::aabb()
  : m_min{ FLT_MAX,  FLT_MAX,  FLT_MAX}
  , m_max{-FLT_MIN, -FLT_MAX, -FLT_MAX}
{
}

inline void aabb::expand(const aabb& _bounds) {
  expand(_bounds.m_min);
  expand(_bounds.m_max);
}

inline const vec3f& aabb::min() const & {
  return m_min;
}

inline const vec3f& aabb::max() const & {
  return m_max;
}

inline vec3f aabb::origin() const {
  return (m_min + m_max) * 0.5f;
}

inline vec3f aabb::scale() const {
  return (m_max - m_min) * 0.5f;
}

}

#endif // RX_MATH_AABB_H