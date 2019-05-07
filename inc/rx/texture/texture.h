#ifndef RX_TEXTURE_TEXTURE_H
#define RX_TEXTURE_TEXTURE_H

#include <rx/texture/loader.h>
#include <rx/math/vec4.h>

namespace rx::texture {

struct loader;

struct texture {
  enum class pixel_format {
    k_rgba_u8,
    k_bgra_u8,
    k_rgb_u8,
    k_bgr_u8,
    k_r_u8
  };

  struct level {
    rx_size offset;
    rx_size size;
    math::vec2z dimensions;
  };

  // create texture with given |_loader| data
  texture(loader&& _loader, bool _want_mipchain);

  texture(array<rx_byte>&& _data, pixel_format _format,
    const math::vec2z& _dimensions, bool _has_mipchain, bool _want_mipchain);
  texture(const rx_byte* _data, pixel_format _format,
    const math::vec2z& _dimensions, bool _has_mipchain, bool _want_mipchain);

  texture(memory::allocator* _allocator, const rx_byte* _data,
    pixel_format _format, const math::vec2z& _dimensions,
    bool _has_mipchain, bool _want_mipchain);

  void resize(const math::vec2z& _dimensions);

  array<rx_byte>&& data() &&;
  const array<rx_byte>& data() const &;
  const array<level>& levels() const &;
  const math::vec2z& dimensions() const &;
  pixel_format format() const;
  rx_size bpp() const;

private:
  void generate_mipchain(bool _has_mipchain, bool _want_mipchain);

  static pixel_format pixel_format_for_loader_bpp(rx_size _bpp);

  array<rx_byte> m_data;
  array<level> m_levels;
  math::vec2z m_dimensions;
  pixel_format m_pixel_format;
};

inline texture::pixel_format texture::pixel_format_for_loader_bpp(rx_size _bpp) {
  switch (_bpp) {
  case 4:
    return pixel_format::k_rgba_u8;
  case 3:
    return pixel_format::k_rgb_u8;
  case 1:
    return pixel_format::k_r_u8;
  }

  RX_UNREACHABLE();
}

inline texture::texture(loader&& _loader, bool _want_mipchain)
  : texture{utility::move(_loader).data(),
      pixel_format_for_loader_bpp(_loader.bpp()), _loader.dimensions(),
      false, _want_mipchain}
{
}

inline texture::texture(const rx_byte* _data, pixel_format _pixel_format,
    const math::vec2z& _dimensions, bool _has_mipchain, bool _want_mipchain)
  : texture{&memory::g_system_allocator, _data, _pixel_format, _dimensions,
      _has_mipchain, _want_mipchain}
{
}

inline array<rx_byte>&& texture::data() && {
  return utility::move(m_data);
}

inline const array<rx_byte>& texture::data() const & {
  return m_data;
}

inline const array<texture::level>& texture::levels() const & {
  return m_levels;
}

inline const math::vec2z& texture::dimensions() const & {
  return m_dimensions;
}

inline texture::pixel_format texture::format() const {
  return m_pixel_format;
}

inline rx_size texture::bpp() const {
  switch (m_pixel_format) {
  case pixel_format::k_r_u8:
    return 1;
  case pixel_format::k_bgr_u8:
    [[fallthrough]];
  case pixel_format::k_rgb_u8:
    return 3;
  case pixel_format::k_bgra_u8:
    [[fallthrough]];
  case pixel_format::k_rgba_u8:
    return 4;
  }

  return 0;
}

} // namespace rx::texture

#endif // RX_TEXTURE_TEXTURE_H