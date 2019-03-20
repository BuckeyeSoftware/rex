#include <stdio.h> // snprintf
#include <math.h> // sqrtf

#include <rx/math/vec2.h> // vec2

namespace rx {

const char* format<math::vec2f>::operator()(const math::vec2f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f}", value.x, value.y);
  return scratch;
}

const char* format<math::vec2i>::operator()(const math::vec2i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d}", value.x, value.y);
  return scratch;
}

rx_f32 math::length(const vec2f& _value) {
  return sqrtf(dot(_value, _value));
}

math::vec2f math::normalize(const math::vec2f& _value) {
  const rx_f32 l{length(_value)};
  return {_value.x / l, _value.y / l};
}

} // namespace rx
