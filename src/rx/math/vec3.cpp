#include <stdio.h> // snprintf

#include "rx/math/trig.h"
#include "rx/math/vec3.h" // vec3

namespace rx {

const char* format_type<math::vec3f>::operator()(const math::vec3f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f}", value.x, value.y, value.z);
  return scratch;
}

const char* format_type<math::vec3i>::operator()(const math::vec3i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d}", value.x, value.y, value.z);
  return scratch;
}

rx_f32 math::length(const math::vec3f& _value) {
  return sqrt(dot(_value, _value));
}

math::vec3f math::normalize(const math::vec3f& _value) {
  return _value * (1.0f / math::length(_value));
}

} // namespace rx
