#include "rx/console/variable.h"
#include "rx/console/console.h"

namespace rx::console {

variable_reference::variable_reference(const char* name,
  const char* description, void* handle, variable_type type)
  : m_name{name}
  , m_description{description}
  , m_handle{handle}
  , m_type{type}
{
  m_next = console::add_variable_reference(this);
}

// instance for all console variable types here, we don't instance for
// bool or string since those are already explicitly specialized
template struct variable<rx_s32>;
template struct variable<rx_f32>;
template struct variable<vec2i>;
template struct variable<vec2f>;
template struct variable<vec3i>;
template struct variable<vec3f>;
template struct variable<vec4i>;
template struct variable<vec4f>;

} // namespace rx::console
