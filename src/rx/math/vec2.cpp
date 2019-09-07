#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

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

} // namespace rx
