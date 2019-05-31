#include <stdio.h> // snprintf

#include "rx/math/trig.h" 
#include "rx/math/vec4.h" // vec4

namespace rx {

const char* format_type<math::vec4f>::operator()(const math::vec4f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f, %f}", value.x, value.y,
    value.z, value.w);
  return scratch;
}

const char* format_type<math::vec4i>::operator()(const math::vec4i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d, %d}", value.x, value.y,
    value.z, value.w);
  return scratch;
}

rx_f32 math::length(const math::vec4f& _value) {
  return sqrt(dot(_value, _value));
}

math::vec4f math::normalize(const math::vec4f& _value) {
  return _value * (1.0f / math::length(_value));
}

} // namespace rx
