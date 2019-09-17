#include "rx/math/aabb.h"

namespace rx::math {

void aabb::expand(const vec3f& _point) {
  m_min.x = algorithm::min(_point.x, m_min.x);
  m_min.y = algorithm::min(_point.y, m_min.y);
  m_min.z = algorithm::min(_point.z, m_min.z);

  m_max.x = algorithm::max(_point.x, m_max.x);
  m_max.y = algorithm::max(_point.y, m_max.y);
  m_max.z = algorithm::max(_point.z, m_max.z);
}

} // namespace rx::math