#include <rx/render/resource.h>
#include <rx/render/frontend.h>

#include <rx/core/log.h>
#include <rx/core/concurrency/scope_lock.h>

RX_LOG("render/resource", log_resource);

namespace rx::render {

static constexpr const char* resource_type_to_string(resource::type _type) {
  switch (_type) {
  case resource::type::k_buffer:
    return "buffer";
  case resource::type::k_program:
    return "program";
  case resource::type::k_target:
    return "target";
  case resource::type::k_texture1D:
    return "texture1D";
  case resource::type::k_texture2D:
    return "texture2D";
  case resource::type::k_texture3D:
    return "texture3D";
  case resource::type::k_textureCM:
    return "textureCM";
  }
  return "";
}

resource::resource(frontend* _frontend, type _type)
  : m_frontend{_frontend}
  , m_resource_type{_type}
{
  log_resource(log::level::k_verbose, "%p init %s", this, resource_type_to_string(_type));
}

resource::~resource() {
  update_resource_usage(0);

  log_resource(log::level::k_verbose, "%p fini %s", this, resource_type_to_string(m_resource_type));
}

void resource::update_resource_usage(rx_size _bytes) {
  m_frontend->m_resource_usage[0][static_cast<rx_size>(m_resource_type)] = _bytes;
}

} // namespace rx::render
