#ifndef RX_MATH_TRANSFORM_H
#define RX_MATH_TRANSFORM_H

#include <rx/math/mat3x3.h> // mat3x3, vec3f
#include <rx/math/mat4x4.h> // mat4x4

namespace rx::math {

struct transform {
  constexpr transform();
  constexpr transform(transform* parent);
  mat3x3f to_mat3() const;
  mat4x4f to_mat4() const;
  vec3f scale, rotate, translate;
  transform *parent;
};

inline constexpr transform::transform()
  : scale{}
  , rotate{}
  , translate{}
  , parent{nullptr}
{
}

inline constexpr transform::transform(transform* parent)
  : scale{}
  , rotate{}
  , translate{}
  , parent{parent}
{
}

inline mat3x3f transform::to_mat3() const {
  const auto local = mat3x3f::scale(scale) * mat3x3f::rotate(rotate) * mat3x3f::translate(translate);
  return parent ? local * parent->to_mat3() : local;
}

inline mat4x4f transform::to_mat4() const {
  const auto mat = to_mat3();
  return {{mat.x.x, mat.x.y, mat.x.z, 0},
          {mat.y.x, mat.y.y, mat.y.z, 0},
          {mat.z.x, mat.z.y, mat.z.z, 0},
          {0,       0,       0,       1}};
}

} // namespace rx::math

#endif // RX_MATH_TRANSFORM_H
