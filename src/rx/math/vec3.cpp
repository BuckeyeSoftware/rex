#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

#include "rx/math/vec3.h" // Vec3

namespace Rx {

const char* FormatNormalize<Math::Vec3f>::operator()(const Math::Vec3f& _value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f}", _value.x, _value.y, _value.z);
  return scratch;
}

const char* FormatNormalize<Math::Vec3i>::operator()(const Math::Vec3i& _value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d}", _value.x, _value.y, _value.z);
  return scratch;
}

const char* FormatNormalize<Math::Vec3z>::operator()(const Math::Vec3z& _value) {
  snprintf(scratch, sizeof scratch, "{%zu, %zu, %zu}", _value.x, _value.y, _value.z);
  return scratch;
}

} // namespace Rx
