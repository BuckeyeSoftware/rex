#include <stdio.h> // snprintf

#include <rx/math/vec3.h> // vec3

const char* rx::format<rx::math::vec3f>::operator()(const rx::math::vec3f& value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f}", value.x, value.y, value.z);
  return scratch;
}

const char* rx::format<rx::math::vec3i>::operator()(const rx::math::vec3i& value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d}", value.x, value.y, value.z);
  return scratch;
}
