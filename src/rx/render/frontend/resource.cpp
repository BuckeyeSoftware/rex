#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/interface.h"

#include "rx/core/log.h"
#include "rx/core/concurrency/scope_lock.h"

RX_LOG("render/resource", logger);

namespace rx::render::frontend {

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

resource::resource(interface* _frontend, type _type)
  : m_frontend{_frontend}
  , m_resource_type{_type}
  , m_resource_usage{0}
  , m_reference_count{1}
{
  logger->verbose("%p init %s", this, resource_type_to_string(m_resource_type));
}

resource::~resource() {
  const auto index{static_cast<rx_size>(m_resource_type)};
  m_frontend->m_resource_usage[index] -= m_resource_usage;

  logger->verbose("%p fini %s", this, resource_type_to_string(m_resource_type));
}

void resource::update_resource_usage(rx_size _bytes) {
  const auto index{static_cast<rx_size>(m_resource_type)};
  m_frontend->m_resource_usage[index] -= m_resource_usage;
  m_resource_usage = _bytes;
  m_frontend->m_resource_usage[index] += m_resource_usage;
}

} // namespace rx::render::frontend
