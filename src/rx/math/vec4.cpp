#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

#include "rx/math/vec4.h" // vec4

namespace Rx {

const char* FormatNormalize<Math::Vec4f>::operator()(const Math::Vec4f& _value) {
  snprintf(scratch, sizeof scratch, "{%f, %f, %f, %f}", _value.x, _value.y,
    _value.z, _value.w);
  return scratch;
}

const char* FormatNormalize<Math::Vec4i>::operator()(const Math::Vec4i& _value) {
  snprintf(scratch, sizeof scratch, "{%d, %d, %d, %d}", _value.x, _value.y,
    _value.z, _value.w);
  return scratch;
}

const char* FormatNormalize<Math::Vec4z>::operator()(const Math::Vec4z& _value) {
  snprintf(scratch, sizeof scratch, "{%zu, %zu, %zu, %zu}", _value.x, _value.y,
    _value.z, _value.w);
  return scratch;
}

} // namespace Rx
