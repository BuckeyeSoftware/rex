#include <rx/console/variable.h>

namespace rx::console {

static variable_reference* g_head;

variable_reference::variable_reference(const char* name,
  const char* description, void* handle, variable_type type)
  : m_name{name}
  , m_description{description}
  , m_type{type}
{
  m_next = g_head;
  g_head = this;
}

// instance for all console variable types here
template struct variable<rx_s32>;
template struct variable<rx_f32>;
template struct variable<bool>;
template struct variable<string>;
template struct variable<vec2i>;
template struct variable<vec2f>;
template struct variable<vec3i>;
template struct variable<vec3f>;
template struct variable<vec4i>;
template struct variable<vec4f>;

} // namespace rx::console
