#include <rx/texture/texture.h>
#include <rx/texture/scale.h>

#include <rx/math/log2.h>

namespace rx::texture {

texture::texture(array<rx_byte>&& _data, pixel_format _pixel_format,
    const math::vec2z& _dimensions, bool _has_mipchain, bool _want_mipchain)
  : m_data{utility::move(_data)}
  , m_dimensions{_dimensions}
  , m_pixel_format{_pixel_format}
{
  generate_levels(_has_mipchain, _want_mipchain);
}

void texture::generate_levels(bool _has_mipchain, bool _want_mipchain) {
  if (_want_mipchain) {
    const rx_size levels{math::log2(m_dimensions.max_element()) + 1};

    math::vec2z dimensions{m_dimensions};
    rx_size offset{0};
    for (rx_size i{0}; i < levels; i++) {
      const rx_size size{dimensions.area() * bpp()};
      m_levels.push_back({offset, size, dimensions});
      offset += size;
      dimensions /= 2;
    };
  } else {
    m_levels.push_back({0, m_dimensions.area() * bpp(), m_dimensions});
  }

  // eliminate mipchain data
  if (_has_mipchain && !_want_mipchain) {
    m_data.resize(m_dimensions.area() * bpp());
  }

  if (!_has_mipchain && _want_mipchain) {
    // calculate storage needed for mipchain
    rx_size size{0};
    m_levels.each_fwd([&size](const level& _level) { size += _level.size; });
    m_data.resize(size);

    generate_mipmaps();
  }
}

void texture::generate_mipmaps() {
  for (rx_size i = 1; i < m_levels.size(); i++) {
    const auto& src_level{m_levels[i-1]};
    const auto& dst_level{m_levels[i]};

    const rx_byte* const src_data{m_data.data() + src_level.offset};
    rx_byte* const dst_data{m_data.data() + dst_level.offset};

    scale(src_data, src_level.dimensions.w, src_level.dimensions.h,
      bpp(), src_level.dimensions.w * bpp(), dst_data, dst_level.dimensions.w,
      dst_level.dimensions.h);
  }
}

void texture::convert(pixel_format _pixel_format,
  const math::vec4f& _fill_pattern)
{
  if (m_pixel_format == _pixel_format) {
    return;
  }

  // TODO(dweiler): implement
  (void)_fill_pattern;

  // use inplace swap for the following conversions
  //   rgb_u8  -> bgr_u8
  //   bgr_u8  -> rgb_u8
  //
  //   rgba_u8 -> bgra_u8
  //   bgra_u8 -> rgba_u8

  // need temporary storage for the following conversions
  //   rgb_u8   -> rgba_u8
  //   rgb_u8   -> bgra_u8
  //   rgb_u8   -> r_u8
  //
  //   bgr_u8   -> rgba_u8
  //   bgr_u8   -> bgra_u8
  //   bgr_u8   -> r_u8
  //
  //   rgba_u8  -> rgb_u8
  //   rgba_u8  -> bgr_u8
  //   rgba_u8  -> r_u8
  //
  //   bgra_u8  -> rgb_u8
  //   bgra_u8  -> bgr_u8
  //   bgra_u8  -> r_u8
  //
  //   r_u8     -> rgba_u8
  //   r_u8     -> bgra_u8
  //   r_u8     -> rgb_u8
  //   r_u8     -> bgr_u8

  m_pixel_format = _pixel_format;
}

} // namespace rx::texture