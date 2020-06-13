#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/context.h"

#include "rx/core/log.h"
#include "rx/core/concurrency/scope_lock.h"

RX_LOG("render/resource", logger);

namespace Rx::Render::Frontend {

static constexpr const char* resource_type_to_string(Resource::Type _type) {
  switch (_type) {
  case Resource::Type::k_buffer:
    return "buffer";
  case Resource::Type::k_program:
    return "program";
  case Resource::Type::k_target:
    return "target";
  case Resource::Type::k_texture1D:
    return "texture1D";
  case Resource::Type::k_texture2D:
    return "texture2D";
  case Resource::Type::k_texture3D:
    return "texture3D";
  case Resource::Type::k_textureCM:
    return "textureCM";
  }
  return "";
}

Resource::Resource(Context* _frontend, Type _type)
  : m_frontend{_frontend}
  , m_resource_type{_type}
  , m_resource_usage{0}
  , m_reference_count{1}
{
  logger->verbose("%p init %s", this, resource_type_to_string(m_resource_type));
}

Resource::~Resource() {
  const auto index{static_cast<Size>(m_resource_type)};
  m_frontend->m_resource_usage[index] -= m_resource_usage;

  logger->verbose("%p fini %s", this, resource_type_to_string(m_resource_type));
}

void Resource::update_resource_usage(Size _bytes) {
  const auto index{static_cast<Size>(m_resource_type)};
  m_frontend->m_resource_usage[index] -= m_resource_usage;
  m_resource_usage = _bytes;
  m_frontend->m_resource_usage[index] += m_resource_usage;
}

} // namespace rx::render::frontend
