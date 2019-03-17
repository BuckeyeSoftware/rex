#include <string.h> // memcpy

#include <rx/render/texture.h>

namespace rx::render {

texture::texture(frontend* _frontend, resource::category _type)
  : resource{_frontend, _type}
  , m_recorded{0}
{
}

void texture::record_format(data_format _format) {
  RX_ASSERT(!(m_recorded & k_format), "format already recorded");

  m_format = _format;
  m_recorded |= k_format;
}

void texture::record_wrap(const wrap_options& _options) {
  RX_ASSERT(!(m_recorded & k_wrap), "wrap already recorded");

  m_wrap = _options;
  m_recorded |= k_wrap;
}

void texture::record_filter(const filter_options& _options) {
  RX_ASSERT(!(m_recorded & k_filter), "filter already recorded");

  m_filter = _options;
  m_recorded |= k_filter;
}

bool texture::validate() const {
  if (!(m_recorded & k_format)) {
    RX_MESSAGE("texture %p validation failed: format not supplied", this);
    return false;

  }
  if (!(m_recorded & k_filter)) {
    RX_MESSAGE("texture %p validation failed: filter not supplied", this);
    return false;
  }

  if (!(m_recorded & k_wrap)) {
    RX_MESSAGE("texture %p validation failed: wrap not supplied", this);
    return false;
  }

  if (!(m_recorded & k_dimensions)) {
    RX_MESSAGE("texture %p validation failed: dimensions not supplied", this);
    return false;
  }

  return true;
}

// texture1D
texture1D::texture1D(frontend* _frontend)
  : texture{_frontend, resource::category::k_texture1D}
{
}

texture1D::~texture1D() {
}

void texture1D::write(const rx_byte* _data, rx_size _dimensions) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  record_dimensions(_dimensions);

  m_data.resize(byte_size_of_format(m_format) * m_dimensions);
  update_resource_usage(m_data.size());
  memcpy(m_data.data(), _data, m_data.size());
}

void texture1D::record_dimensions(rx_size _dimensions) {
  if (m_recorded & k_dimensions) {
    RX_ASSERT(m_dimensions == _dimensions, "dimensions changed");
  }
  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;
}

texture2D::texture2D(frontend* _frontend)
  : texture{_frontend, resource::category::k_texture2D}
{
}

texture2D::~texture2D() {
}

void texture2D::write(const rx_byte* _data, const math::vec2z& _dimensions) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  record_dimensions(_dimensions);

  m_data.resize(byte_size_of_format(m_format) * m_dimensions.x * m_dimensions.y);
  update_resource_usage(m_data.size());
  memcpy(m_data.data(), _data, m_data.size());
}

void texture2D::record_dimensions(const math::vec2z& _dimensions) {
  if (m_recorded & k_dimensions) {
    RX_ASSERT(_dimensions == m_dimensions, "dimensions changed");
  }
  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;
}

// texture3D
texture3D::texture3D(frontend* _frontend)
  : texture{_frontend, resource::category::k_texture3D}
{
}

texture3D::~texture3D() {
}

void texture3D::write(const rx_byte* _data, const math::vec3z& _dimensions) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  record_dimensions(_dimensions);

  m_data.resize(byte_size_of_format(m_format) * m_dimensions.x * m_dimensions.y * m_dimensions.z);
  update_resource_usage(m_data.size());
  memcpy(m_data.data(), _data, m_data.size());
}

void texture3D::record_dimensions(const math::vec3z& _dimensions) {
  if (m_recorded & k_dimensions) {
    RX_ASSERT(m_dimensions == _dimensions, "dimensions changed");
  }
  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;
}

// textureCM
textureCM::textureCM(frontend* _frontend)
  : texture{_frontend, resource::category::k_textureCM}
{
}

textureCM::~textureCM() {
}

void textureCM::write(const rx_byte* _data, const math::vec2z& _dimensions, face _face) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  record_dimensions(_dimensions);

  // faces are stored as a "film-strip" of 2D textures in memory
  const auto face_size{byte_size_of_format(m_format) * m_dimensions.x * m_dimensions.y};
  m_data.resize(face_size * 6);
  update_resource_usage(m_data.size());
  memcpy(m_data.data() + face_size*static_cast<rx_size>(_face), _data, face_size);
}

void textureCM::record_dimensions(const math::vec2z& _dimensions) {
  if (m_recorded & k_dimensions) {
    RX_ASSERT(_dimensions == m_dimensions, "face dimension changed");
  }
  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;
}

} // namespace rx::render
