#include <string.h> // memcpy

#include "rx/render/frontend/texture.h"
#include "rx/core/math/log2.h"

namespace rx::render::frontend {

texture::texture(interface* _frontend, resource::type _type)
  : resource{_frontend, _type}
  , m_recorded{0}
{
}

void texture::record_format(data_format _format) {
  RX_ASSERT(!(m_recorded & k_format), "format already recorded");

  m_format = _format;
  m_recorded |= k_format;
}

void texture::record_type(type _type) {
  RX_ASSERT(!(m_recorded & k_type), "type already recorded");

  m_type = _type;
  m_recorded |= k_type;
}

void texture::record_filter(const filter_options& _options) {
  RX_ASSERT(!(m_recorded & k_filter), "filter already recorded");

  m_filter = _options;
  m_recorded |= k_filter;
}

void texture::validate() const {
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  RX_ASSERT(m_recorded & k_type, "type not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_wrap, "wrap not recorded");
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
}

// texture1D
texture1D::texture1D(interface* _frontend)
  : texture{_frontend, resource::type::k_texture1D}
{
}

texture1D::~texture1D() {
}

void texture1D::write(const rx_byte* _data, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  const auto& info{m_levels[_level]};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

rx_byte* texture1D::map(rx_size _level) {
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  return m_data.data() + m_levels[_level].offset;
}

void texture1D::record_dimensions(const dimension_type& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_recorded & k_type, "type not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");

  RX_ASSERT(!is_compressed_format(), "1D textures cannot be compressed");

  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;

  if (m_type != type::k_attachment) {
    dimension_type dimensions{m_dimensions};
    rx_size offset{0};
    const auto bpp{byte_size_of_format(m_format)};
    for (rx_size i{0}; i < levels(); i++) {
      const auto size{static_cast<rx_size>(dimensions * bpp)};
      m_levels.push_back({offset, size, dimensions});
      offset += size;
      dimensions = algorithm::max(dimensions / 2, 1_z);
    }

    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void texture1D::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_recorded & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_recorded |= k_wrap;
}

// texture2D
texture2D::texture2D(interface* _frontend)
  : texture{_frontend, resource::type::k_texture2D}
{
}

texture2D::~texture2D() {
}

void texture2D::write(const rx_byte* _data, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  const auto& info{m_levels[_level]};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

rx_byte* texture2D::map(rx_size _level) {
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  return m_data.data() + m_levels[_level].offset;
}

void texture2D::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_recorded & k_type, "type not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;

  if (m_type != type::k_attachment) {
    dimension_type dimensions{m_dimensions};
    rx_size offset{0};
    const auto bpp{byte_size_of_format(m_format)};
    for (rx_size i{0}; i < levels(); i++) {
      const auto size{static_cast<rx_size>(dimensions.area() * bpp)};
      m_levels.push_back({offset, size, dimensions});
      offset += size;
      dimensions = dimensions.map([](rx_size _dim) {
        return algorithm::max(_dim / 2, 1_z);
      });
    }

    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void texture2D::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_recorded & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_recorded |= k_wrap;
}

// texture3D
texture3D::texture3D(interface* _frontend)
  : texture{_frontend, resource::type::k_texture3D}
{
}

texture3D::~texture3D() {
}

void texture3D::write(const rx_byte* _data, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  const auto& info{m_levels[_level]};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

rx_byte* texture3D::map(rx_size _level) {
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  return m_data.data() + m_levels[_level].offset;
}

void texture3D::record_dimensions(const math::vec3z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_recorded & k_type, "type not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");

  RX_ASSERT(!is_compressed_format(), "3D textures cannot be compressed");

  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;

  if (m_type != type::k_attachment) {
    dimension_type dimensions{m_dimensions};
    rx_size offset{0};
    const auto bpp{byte_size_of_format(m_format)};
    for (rx_size i{0}; i < levels(); i++) {
      const auto size{static_cast<rx_size>(dimensions.area() * bpp)};
      m_levels.push_back({offset, size, dimensions});
      offset += size;
      dimensions = dimensions.map([](rx_size _dim) {
        return algorithm::max(_dim / 2, 1_z);
      });
    }

    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void texture3D::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_recorded & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_recorded |= k_wrap;
}

// textureCM
textureCM::textureCM(interface* _frontend)
  : texture{_frontend, resource::type::k_textureCM}
{
}

textureCM::~textureCM() {
}

void textureCM::write(const rx_byte* _data, face _face, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  memcpy(map(_level, _face), _data, m_levels[_level].size / 6);
}

rx_byte* textureCM::map(rx_size _level, face _face) {
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");
  validate();

  const auto& info{m_levels[_level]};
  const auto face_size{info.size / 6};
  const auto face_offset{face_size * static_cast<rx_size>(_face)};
  return m_data.data() + info.offset + face_offset;
}

void textureCM::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_recorded & k_type, "type not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_format, "format not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  m_dimensions = _dimensions;
  m_recorded |= k_dimensions;

  if (m_type != type::k_attachment) {
    dimension_type dimensions{m_dimensions};
    rx_size offset{0};
    const auto bpp{byte_size_of_format(m_format)};
    for (rx_size i{0}; i < levels(); i++) {
      const auto size{static_cast<rx_size>(dimensions.area() * bpp * 6)};
      m_levels.push_back({offset, size, dimensions});
      offset += size;
      dimensions = dimensions.map([](rx_size _dim) {
        return algorithm::max(_dim / 2, 1_z);
      });
    }

    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void textureCM::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_recorded & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_recorded |= k_wrap;
}

} // namespace rx::render::frontend