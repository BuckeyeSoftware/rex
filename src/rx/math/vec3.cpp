#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

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

} // namespace rx
