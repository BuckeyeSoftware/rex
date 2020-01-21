#ifndef RX_MATH_RECTANGLE_H
#define RX_MATH_RECTANGLE_H
#include "rx/math/vec2.h"

namespace rx::math {

template<typename T>
struct rectangle {
  vec2<T> offset;
  vec2<T> dimensions;

  bool contains(const rectangle& _other) const;
};

template<typename T>
inline bool rectangle<T>::contains(const rectangle& _other) const {
  return _other.offset + _other.dimensions < offset + dimensions && _other.offset > offset;
}

} // namespace rx::math

#endif // RX_MATH_RECTANGLE_H
