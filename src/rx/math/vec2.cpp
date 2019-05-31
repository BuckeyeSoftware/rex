#include <stdio.h> // snprintf

#include "rx/math/trig.h"
#include "rx/math/vec2.h"

namespace rx {

const char* format_type<math::vec2f>::operator()(const math::vec2f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f}", value.x, value.y);
  return scratch;
}

const char* format_type<math::vec2i>::operator()(const math::vec2i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d}", value.x, value.y);
  return scratch;
}

rx_f32 math::length(const vec2f& _value) {
  return sqrt(dot(_value, _value));
}

math::vec2f math::normalize(const math::vec2f& _value) {
  return _value * (1.0f / math::length(_value));
}

} // namespace rx
