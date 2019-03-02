#include <stdio.h> // snprintf

#include <rx/math/vec4.h> // vec4

const char* rx::format<rx::math::vec4f>::operator()(const rx::math::vec4f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f, %f}", value.x, value.y, value.z, value.w);
  return scratch;
}

const char* rx::format<rx::math::vec4i>::operator()(const rx::math::vec4i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d, %d}", value.x, value.y, value.z, value.w);
  return scratch;
}
