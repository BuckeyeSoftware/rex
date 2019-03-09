#include <stdio.h> // snprintf

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

} // namespace rx
