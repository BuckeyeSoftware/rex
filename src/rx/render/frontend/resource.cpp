#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render::Frontend {

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
