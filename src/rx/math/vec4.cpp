#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

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

} // namespace rx
