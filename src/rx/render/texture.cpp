#include <string.h> // memcpy

#include <rx/render/texture.h>
#include <rx/math/log2.h>

namespace rx::render {

struct level_extents {
  rx_size offset;
  rx_size size;
};

inline rx_size storage_for_1D(rx_size _dimensions, rx_size _bpp, bool _mips) {
  if (_mips) {
    rx_size size{0};
    const rx_size levels{math::log2(_dimensions)+1};
    for (rx_size i{0}; i < levels; i++) {
      size += _dimensions * _bpp;
      _dimensions >>= 1;
    }
    return size;
  }
  return _dimensions * _bpp;
}

inline rx_size storage_for_2D(math::vec2z _dimensions, rx_size _bpp, bool _mips) {
  if (_mips) {
    rx_size size{0};
    const rx_size levels{math::log2(algorithm::max(_dimensions.x, _dimensions.y))+1};
    for (rx_size i{0}; i < levels; i++) {
      size += _dimensions.area() * _bpp;
      _dimensions /= 2;
    }
    return size;
  }
  return _dimensions.area() * _bpp;
}

inline rx_size storage_for_3D(math::vec3z _dimensions, rx_size _bpp, bool _mips) {
  if (_mips) {
    rx_size size{0};
    const rx_size levels{math::log2(algorithm::max(_dimensions.w, _dimensions.h, _dimensions.d))+1};
    for (rx_size i{0}; i < levels; i++) {
      size += _dimensions.area() * _bpp;
      _dimensions /= 2;
    }
    return size;
  }
  return _dimensions.area() * _bpp;
}

inline rx_size storage_for_CM(math::vec2z _dimensions, rx_size _bpp, bool _mips) {
  return storage_for_2D(_dimensions, _bpp, _mips) * 6;
}

inline level_extents extents_for_1D(rx_size _dimensions, rx_size _bpp, rx_size _level) {
  rx_size offset{0};
  rx_size size{0};
  for (rx_size i{0}; i < _level; i++) {
    size = _dimensions * _bpp;
    offset += size;
    _dimensions /= 2;
  }
  return {offset, size};
}

inline level_extents extents_for_2D(math::vec2z _dimensions, rx_size _bpp, rx_size _level) {
  rx_size offset{0};
  rx_size size{0};
  for (rx_size i{0}; i < _level; i++) {
    size = _dimensions.area() * _bpp;
    offset += size;
    _dimensions /= 2;
  }
  return {offset, size};
}

inline level_extents extents_for_3D(math::vec3z _dimensions, rx_size _bpp, rx_size _level) {
  rx_size offset{0};
  rx_size size{0};
  for (rx_size i{0}; i < _level; i++) {
    size = _dimensions.area() * _bpp;
    offset += size;
    _dimensions /= 2;
  }
  return {offset, size};
}

inline level_extents extents_for_CM(math::vec2z _dimensions, rx_size _bpp, rx_size _level, textureCM::face _face) {
  rx_size offset{0};
  rx_size size{0};
  for (rx_size i{0}; i < _level; i++) {
    size = _dimensions.area() * _bpp;
    offset += size * 6;
    _dimensions /= 2;
  }
  return {offset + size * static_cast<rx_size>(_face), size};
}

texture::texture(frontend* _frontend, resource::type _type)
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

void texture::validate() const {
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_wrap, "wrap not recorded");
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");

  if (m_filter.mip_maps) {
    // TODO: ensure all miplevels are recorded
  }
}

// texture1D
texture1D::texture1D(frontend* _frontend)
  : texture{_frontend, resource::type::k_texture1D}
{
}

texture1D::~texture1D() {
}

void texture1D::write(const rx_byte* _data, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(_level && _level <= levels(), "mipmap level out of bounds");

  const auto extents{extents_for_1D(m_dimensions, byte_size_of_format(m_format), _level)};
  memcpy(m_data.data() + extents.offset, _data, extents.size);
}

void texture1D::record_dimensions(rx_size _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");

  m_dimensions = _dimensions;
  m_dimensions_log2 = math::log2(m_dimensions);
  m_data.resize(storage_for_1D(_dimensions, byte_size_of_format(m_format), m_filter.mip_maps));
  update_resource_usage(m_data.size());
  m_recorded |= k_dimensions;
}

texture2D::texture2D(frontend* _frontend)
  : texture{_frontend, resource::type::k_texture2D}
{
}

texture2D::~texture2D() {
}

void texture2D::write(const rx_byte* _data, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(_level && _level <= levels(), "mipmap level out of bounds");

  const auto extents{extents_for_2D(m_dimensions, byte_size_of_format(m_format), _level)};
  memcpy(m_data.data() + extents.offset, _data, extents.size);
}

void texture2D::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");

  m_dimensions = _dimensions;
  m_dimensions_log2 = m_dimensions.map(math::log2<rx_size>);
  m_data.resize(storage_for_2D(_dimensions, byte_size_of_format(m_format), m_filter.mip_maps));
  update_resource_usage(m_data.size());
  m_recorded |= k_dimensions;
}

// texture3D
texture3D::texture3D(frontend* _frontend)
  : texture{_frontend, resource::type::k_texture3D}
{
}

texture3D::~texture3D() {
}

void texture3D::write(const rx_byte* _data, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(_level && _level <= levels(), "mipmap level out of bounds");

  const auto extents{extents_for_3D(m_dimensions, byte_size_of_format(m_format), _level)};
  memcpy(m_data.data() + extents.offset, _data, extents.size);
}

void texture3D::record_dimensions(const math::vec3z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");

  m_dimensions = _dimensions;
  m_dimensions_log2 = m_dimensions.map(math::log2<rx_size>);
  m_data.resize(storage_for_3D(m_dimensions, byte_size_of_format(m_format), m_filter.mip_maps));
  update_resource_usage(m_data.size());
  m_recorded |= k_dimensions;
}

// textureCM
textureCM::textureCM(frontend* _frontend)
  : texture{_frontend, resource::type::k_textureCM}
{
}

textureCM::~textureCM() {
}

void textureCM::write(const rx_byte* _data, face _face, rx_size _level) {
  RX_ASSERT(_data, "_data is null");
  RX_ASSERT(m_recorded & k_format, "format not recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");
  RX_ASSERT(m_recorded & k_dimensions, "dimensions not recorded");
  RX_ASSERT(_level && _level <= levels(), "mipmap level out of bounds");

  // TODO(dweiler): implement
  const auto extents{extents_for_CM(m_dimensions, byte_size_of_format(m_format), _level, _face)};
  memcpy(m_data.data() + extents.offset, _data, extents.size);
}

void textureCM::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  RX_ASSERT(m_recorded & k_filter, "filter not recorded");

  m_dimensions = _dimensions;
  m_dimensions_log2 = m_dimensions.map(math::log2<rx_size>);
  m_data.resize(storage_for_CM(m_dimensions, byte_size_of_format(m_format), m_filter.mip_maps));
  update_resource_usage(m_data.size());
  m_recorded |= k_dimensions;
}

} // namespace rx::render
