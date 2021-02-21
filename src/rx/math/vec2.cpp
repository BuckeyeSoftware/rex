#include <stdio.h> // snprintf

#include "rx/core/math/sqrt.h"

#include "rx/math/vec2.h"

namespace Rx {

const char* FormatNormalize<Math::Vec2f>::operator()(const Math::Vec2f& _value) {
  snprintf(scratch, sizeof scratch, "{%f, %f}", _value.x, _value.y);
  return scratch;
}

const char* FormatNormalize<Math::Vec2i>::operator()(const Math::Vec2i& _value) {
  snprintf(scratch, sizeof scratch, "{%d, %d}", _value.x, _value.y);
  return scratch;
}

const char* FormatNormalize<Math::Vec2z>::operator()(const Math::Vec2z& _value) {
  snprintf(scratch, sizeof scratch, "{%zu, %zu}", _value.x, _value.y);
  return scratch;
}

} // namespace Rx
