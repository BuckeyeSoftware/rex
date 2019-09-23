#ifndef RX_MATH_PLANE_H
#define RX_MATH_PLANE_H
#include "rx/math/vec3.h"

namespace rx::math {

struct plane {
  constexpr plane();
  plane(const vec3f& _normal, rx_f32 _distance);
  plane(const vec3f& _normal, const vec3f& _point);

  const vec3f& normal() const &;
  rx_f32 distance() const;

private:
  vec3f m_normal;
  rx_f32 m_distance;
};

inline constexpr plane::plane()
  : m_normal{}
  , m_distance{0.0f}
{
}

inline plane::plane(const vec3f& _normal, rx_f32 _distance)
  : m_normal{_normal}
  , m_distance{_distance}
{
  const rx_f32 magnitude{1.0f / length(m_normal)};
  m_normal *= magnitude;
  m_distance *= magnitude;
}

inline plane::plane(const vec3f& _normal, const vec3f& _point)
  : m_normal{normalize(_normal)}
  , m_distance{dot(m_normal, _point)}
{
}

inline const vec3f& plane::normal() const & {
  return m_normal;
}

inline rx_f32 plane::distance() const {
  return m_distance;
}

} // namespace rx::math

#endif // RX_MATH_PLANE_H
