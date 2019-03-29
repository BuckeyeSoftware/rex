#include <string.h> // memcpy

#include <rx/render/texture.h>
#include <rx/math/log2.h>

namespace rx::render {

template<typename T>
static inline bool is_pot(T _x) {
  return _x && !(_x & (_x - 1));
}

static inline rx_size offset_for_1D(rx_size _dimensions_log2, rx_size _level, rx_size _bpp) {
  return ((1 << 1*_level) - 1) * (1 << ((_dimensions_log2 - 1 * _level) + 1)) * _bpp;
}

static inline rx_size offset_for_2D(const math::vec2z& _dimensions_log2, rx_size _level, rx_size _bpp) {
  return (((1 << 2*_level) - 1) * (1 << ((_dimensions_log2.w + _dimensions_log2.h - 2 * _level) + 2)) / 3) * _bpp;
}

static inline rx_size offset_for_3D(const math::vec3z& _dimensions_log2, rx_size _level, rx_size _bpp) {
  return (((1 << 3*_level) - 1) * (1 << ((_dimensions_log2.w + _dimensions_log2.h + _dimensions_log2.d - 3 * _level) + 3)) / 7) * _bpp;
}

static inline rx_size offset_for_CM(const math::vec2z& _dimensions_log2, rx_size _level, textureCM::face _face, rx_size _bpp) {
  const rx_size offset2D{offset_for_2D({_dimensions_log2.w, _dimensions_log2.h*6}, _level, _bpp)};
  return offset2D + static_cast<rx_size>(_face) * (1 << (_dimensions_log2.w + _dimensions_log2.h - 2 * _level)) * _bpp;
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
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");

  const rx_size level_dimensions{1_z << (m_dimensions_log2 - _level)};
  const rx_size bpp{byte_size_of_format(m_format)};
  const rx_size size{level_dimensions * bpp};
  m_data.resize(m_dimensions * levels() * byte_size_of_format(m_format));
  memcpy(m_data.data() + offset_for_1D(m_dimensions_log2, _level, bpp), _data, size);
}

void texture1D::record_dimensions(rx_size _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  if (m_filter.mip_maps) {
    RX_ASSERT(is_pot(_dimensions), "mipmaps only supported for dimensions that are POT");
  }

  m_dimensions = _dimensions;
  m_dimensions_log2 = math::log2(m_dimensions);
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
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");

  const math::vec2z level_dimensions{math::vec2z{1} << (m_dimensions_log2 - _level)};
  const rx_size bpp{byte_size_of_format(m_format)};
  const rx_size size{level_dimensions.area() * bpp};
  m_data.resize(m_dimensions.area() * levels() * byte_size_of_format(m_format));
  update_resource_usage(m_data.size());
  memcpy(m_data.data() + offset_for_2D(m_dimensions_log2, _level, bpp), _data, size);
}

void texture2D::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  if (m_filter.mip_maps) {
    RX_ASSERT(is_pot(_dimensions.w) && is_pot(_dimensions.h),
      "mipmaps only supported for dimensions that are POT");
  }

  m_dimensions = _dimensions;
  m_dimensions_log2.w = math::log2(m_dimensions.w);
  m_dimensions_log2.h = math::log2(m_dimensions.h);
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
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");

  const math::vec3z level_dimensions{math::vec3z{1} << (m_dimensions_log2 - _level)};
  const rx_size bpp{byte_size_of_format(m_format)};
  const rx_size size{level_dimensions.area() * bpp};
  m_data.resize(m_dimensions.area() * levels() * byte_size_of_format(m_format));
  update_resource_usage(m_data.size());
  memcpy(m_data.data() + offset_for_3D(m_dimensions_log2, _level, bpp), _data, size);
}

void texture3D::record_dimensions(const math::vec3z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  if (m_filter.mip_maps) {
    RX_ASSERT(is_pot(_dimensions.w) && is_pot(_dimensions.h) &&
      is_pot(_dimensions.d), "mipmaps only supported for dimensions that are POT");
  }

  m_dimensions = _dimensions;
  m_dimensions_log2.w = math::log2(m_dimensions.w);
  m_dimensions_log2.h = math::log2(m_dimensions.y);
  m_dimensions_log2.d = math::log2(m_dimensions.d);
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
  RX_ASSERT(_level < levels(), "mipmap level out of bounds");

  const math::vec2z level_dimensions{math::vec2z{1} << (m_dimensions_log2 - _level)};
  const rx_size bpp{byte_size_of_format(m_format)};
  const rx_size size{level_dimensions.area() * bpp};
  m_data.resize(m_dimensions.area() * levels() * 6 * byte_size_of_format(m_format));
  update_resource_usage(m_data.size());
  memcpy(m_data.data() + offset_for_CM(m_dimensions_log2, _level, _face, bpp), _data, size);
}

void textureCM::record_dimensions(const math::vec2z& _dimensions) {
  RX_ASSERT(!(m_recorded & k_dimensions), "dimensions already recorded");
  if (m_filter.mip_maps) {
    RX_ASSERT(is_pot(_dimensions.w) && is_pot(_dimensions.h),
      "mipmaps only supported for dimensions that are POT");
  }

  m_dimensions = _dimensions;
  m_dimensions_log2.w = math::log2(m_dimensions.w);
  m_dimensions_log2.h = math::log2(m_dimensions.h);
  m_recorded |= k_dimensions;
}

} // namespace rx::render
