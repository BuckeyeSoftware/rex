#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render::Frontend {

static constexpr const char* resource_type_to_string(Resource::Type _type) {
  switch (_type) {
  case Resource::Type::k_buffer:
    return "Buffer";
  case Resource::Type::k_program:
    return "Program";
  case Resource::Type::k_target:
    return "Target";
  case Resource::Type::k_texture1D:
    return "Texture1D";
  case Resource::Type::k_texture2D:
    return "Texture2D";
  case Resource::Type::k_texture3D:
    return "Texture3D";
  case Resource::Type::k_textureCM:
    return "TextureCM";
  case Resource::Type::k_downloader:
    return "Downloader";
  }
  return "";
}

Resource::Resource(Context* _frontend, Type _type)
  : m_frontend{_frontend}
  , m_resource_type{_type}
  , m_resource_usage{0}
  , m_reference_count{1}
{
}

Resource::~Resource() {
  const auto index = static_cast<Size>(m_resource_type);
  m_frontend->m_resource_usage[index] -= m_resource_usage;
}

void Resource::update_resource_usage(Size _bytes) {
  const auto index = static_cast<Size>(m_resource_type);

  m_frontend->m_resource_usage[index] -= m_resource_usage;
  m_resource_usage = _bytes;
  m_frontend->m_resource_usage[index] += m_resource_usage;
}

} // namespace rx::render::frontend
