#include "rx/console/variable.h"
#include "rx/console/interface.h"

#include "rx/core/hints/unreachable.h"

namespace rx::console {

const char* variable_type_as_string(variable_type _type) {
  switch (_type) {
  case variable_type::k_boolean:
    return "bool";
  case variable_type::k_string:
    return "string";
  case variable_type::k_int:
    return "int";
  case variable_type::k_float:
    return "float";
  case variable_type::k_vec4f:
    return "vec4f";
  case variable_type::k_vec4i:
    return "vec4i";
  case variable_type::k_vec3f:
    return "vec3f";
  case variable_type::k_vec3i:
    return "vec3i";
  case variable_type::k_vec2f:
    return "vec2f";
  case variable_type::k_vec2i:
    return "vec2i";
  }
  RX_HINT_UNREACHABLE();
}

variable_reference::variable_reference(const char* name,
  const char* description, void* handle, variable_type type)
  : m_name{name}
  , m_description{description}
  , m_handle{handle}
  , m_type{type}
{
  m_next = interface::add_variable(this);
}

void variable_reference::reset() {
  switch (m_type) {
  case variable_type::k_boolean:
    cast<bool>()->reset();
    break;
  case variable_type::k_string:
    cast<string>()->reset();
    break;
  case variable_type::k_int:
    cast<rx_s32>()->reset();
    break;
  case variable_type::k_float:
    cast<rx_f32>()->reset();
    break;
  case variable_type::k_vec4f:
    cast<math::vec4f>()->reset();
    break;
  case variable_type::k_vec4i:
    cast<math::vec4i>()->reset();
    break;
  case variable_type::k_vec3f:
    cast<math::vec3f>()->reset();
    break;
  case variable_type::k_vec3i:
    cast<math::vec3i>()->reset();
    break;
  case variable_type::k_vec2f:
    cast<math::vec2f>()->reset();
    break;
  case variable_type::k_vec2i:
    cast<math::vec2i>()->reset();
    break;
  }
}

string variable_reference::print_value() const {
  switch (m_type) {
  case variable_type::k_boolean:
    return cast<bool>()->get() ? "true" : "false";
  case variable_type::k_string:
    return string::format("\"%s\"", cast<string>()->get());
  case variable_type::k_int:
    return string::format("%d", cast<rx_s32>()->get());
  case variable_type::k_float:
    return string::format("%f", cast<rx_f32>()->get());
  case variable_type::k_vec4f:
    return string::format("%s", cast<math::vec4f>()->get());
  case variable_type::k_vec4i:
    return string::format("%s", cast<math::vec4i>()->get());
  case variable_type::k_vec3f:
    return string::format("%s", cast<math::vec3f>()->get());
  case variable_type::k_vec3i:
    return string::format("%s", cast<math::vec3i>()->get());
  case variable_type::k_vec2f:
    return string::format("%s", cast<math::vec2f>()->get());
  case variable_type::k_vec2i:
    return string::format("%s", cast<math::vec2i>()->get());
  }
  RX_HINT_UNREACHABLE();
}

string variable_reference::print_range() const {
  // TODO(dweiler): implemenet
  return "";
}

template struct variable<rx_s32>;
template struct variable<rx_f32>;
template struct variable<vec2i>;
template struct variable<vec2f>;
template struct variable<vec3i>;
template struct variable<vec3f>;
template struct variable<vec4i>;
template struct variable<vec4f>;

} // namespace rx::console
