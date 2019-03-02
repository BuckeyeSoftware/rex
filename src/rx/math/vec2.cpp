#include <stdio.h> // snprintf

#include <rx/math/vec2.h> // vec2

const char* rx::format<rx::math::vec2f>::operator()(const rx::math::vec2f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f}", value.x, value.y);
  return scratch;
}

const char* rx::format<rx::math::vec2i>::operator()(const rx::math::vec2i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d}", value.x, value.y);
  return scratch;
}
