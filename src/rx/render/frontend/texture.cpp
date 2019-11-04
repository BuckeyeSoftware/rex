#include <string.h> // memcpy

#include "rx/render/frontend/texture.h"
#include "rx/core/math/log2.h"

namespace rx::render::frontend {

texture::texture(interface* _frontend, resource::type _type)
  : resource{_frontend, _type}
  , m_flags{0}
{
}

void texture::record_format(data_format _format) {
  RX_ASSERT(!(m_flags & k_format), "format already recorded");
  m_format = _format;
  m_flags |= k_format;
}

void texture::record_type(type _type) {
  RX_ASSERT(!(m_flags & k_type), "type already recorded");
  m_type = _type;
  m_flags |= k_type;
}

void texture::record_filter(const filter_options& _options) {
  RX_ASSERT(!(m_flags & k_filter), "filter already recorded");
  m_filter = _options;
  m_flags |= k_filter;
}

void texture::record_levels(rx_size _levels) {
  RX_ASSERT(!(m_flags & k_levels), "levels already recorded");
  RX_ASSERT(_levels, "_levels must be at least 1");

  m_levels = _levels;
  m_flags |= k_levels;
}

void texture::record_border(const math::vec4f& _border) {
  RX_ASSERT(!(m_flags & k_border), "border already recorded");
  RX_ASSERT(m_flags & k_wrap, "wrap not recorded");

  m_border = _border;
  m_flags |= k_border;
}

void texture::validate() const {
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_type, "type not recorded");
  RX_ASSERT(m_flags & k_filter, "filter not recorded");
  RX_ASSERT(m_flags & k_wrap, "wrap not recorded");
  RX_ASSERT(m_flags & k_dimensions, "dimensions not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  if (m_filter.mipmaps) {
    RX_ASSERT(m_levels > 1, "no levels specified for mipmaps");
  }

  if (m_flags & k_swapchain) {
    RX_ASSERT(!m_filter.mipmaps, "swapchain cannot have mipmaps");
    RX_ASSERT(!m_filter.bilinear, "swapchain cannot have bilinear filtering");
    RX_ASSERT(!m_filter.trilinear, "swapchain cannot have trilinear filtering");
    RX_ASSERT(m_levels == 1, "swapchain cannot have levels");
  }
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
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");

  const auto& info{info_for_level(_level)};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

rx_byte* texture1D::map(rx_size _level) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  return m_data.data() + info.offset;
}

void texture1D::record_dimensions(const dimension_type& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  RX_ASSERT(!is_compressed_format(), "1D textures cannot be compressed");

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  dimension_type dimensions{m_dimensions};
  rx_size offset{0};
  const auto bpp{byte_size_of_format(m_format)};
  for (rx_size i{0}; i < m_levels; i++) {
    const auto size{static_cast<rx_size>(dimensions * bpp)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = algorithm::max(dimensions / 2, 1_z);
  }

  if (m_type != type::k_attachment) {
    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void texture1D::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
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
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

rx_byte* texture2D::map(rx_size _level) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  return m_data.data() + info.offset;
}

void texture2D::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  dimension_type dimensions{m_dimensions};
  rx_size offset{0};
  const auto bpp{byte_size_of_format(m_format)};
  for (rx_size i{0}; i < m_levels; i++) {
    const auto size{static_cast<rx_size>(dimensions.area() * bpp)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = dimensions.map([](rx_size _dim) {
      return algorithm::max(_dim / 2, 1_z);
    });
  }

  if (m_type != type::k_attachment) {
    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void texture2D::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
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
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  memcpy(m_data.data() + info.offset, _data, info.size);
}

rx_byte* texture3D::map(rx_size _level) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  return m_data.data() + info.offset;
}

void texture3D::record_dimensions(const math::vec3z& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  RX_ASSERT(!is_compressed_format(), "3D textures cannot be compressed");

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  dimension_type dimensions{m_dimensions};
  rx_size offset{0};
  const auto bpp{byte_size_of_format(m_format)};
  for (rx_size i{0}; i < m_levels; i++) {
    const auto size{static_cast<rx_size>(dimensions.area() * bpp)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = dimensions.map([](rx_size _dim) {
      return algorithm::max(_dim / 2, 1_z);
    });
  }

  if (m_type != type::k_attachment) {
    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void texture3D::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
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
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  memcpy(map(_level, _face), _data, info.size / 6);
}

rx_byte* textureCM::map(rx_size _level, face _face) {
  RX_ASSERT(is_level_in_range(_level), "mipmap level out of bounds");
  validate();

  const auto& info{info_for_level(_level)};
  const auto face_size{info.size / 6};
  const auto face_offset{face_size * static_cast<rx_size>(_face)};
  return m_data.data() + info.offset + face_offset;
}

void textureCM::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_flags & k_dimensions), "dimensions already recorded");

  RX_ASSERT(m_flags & k_type, "type not recorded");
  RX_ASSERT(m_flags & k_format, "format not recorded");
  RX_ASSERT(m_flags & k_levels, "levels not recorded");

  if (is_compressed_format()) {
    RX_ASSERT(_dimensions.w >= 4 && _dimensions.h >= 4,
      "too small for compression");
  }

  m_dimensions = _dimensions;
  m_flags |= k_dimensions;

  dimension_type dimensions{m_dimensions};
  rx_size offset{0};
  const auto bpp{byte_size_of_format(m_format)};
  for (rx_size i{0}; i < m_levels; i++) {
    const auto size{static_cast<rx_size>(dimensions.area() * bpp * 6)};
    m_level_info.push_back({offset, size, dimensions});
    offset += size;
    dimensions = dimensions.map([](rx_size _dim) {
      return algorithm::max(_dim / 2, 1_z);
    });
  }

  if (m_type != type::k_attachment) {
    m_data.resize(offset);
    update_resource_usage(m_data.size());
  }
}

void textureCM::record_wrap(const wrap_options& _wrap) {
  RX_ASSERT(!(m_flags & k_wrap), "wrap already recorded");

  m_wrap = _wrap;
  m_flags |= k_wrap;
}

} // namespace rx::render::frontend
