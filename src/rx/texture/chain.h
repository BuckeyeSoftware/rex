#ifndef RX_TEXTURE_CHAIN_H
#define RX_TEXTURE_CHAIN_H
#include "rx/texture/loader.h"

#include "rx/core/hints/unreachable.h"
#include "rx/core/concepts/no_copy.h"

namespace rx::texture {

struct loader;

struct chain
  : concepts::no_copy
{
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

  chain();
  chain(memory::allocator* _allocator);
  chain(chain&& _chain);

  chain& operator=(chain&& _chain);

  void generate(loader&& _loader, bool _has_mipchain, bool _want_mipchain);

  void generate(vector<rx_byte>&& _data, pixel_format _format,
    const math::vec2z& _dimensions, bool _has_mipchain, bool _want_mipchain);
  void generate(const rx_byte* _data, pixel_format _format,
    const math::vec2z& _dimensions, bool _has_mipchain, bool _want_mipchain);

  void resize(const math::vec2z& _dimensions);

  vector<rx_byte>&& data() &&;
  vector<level>&& levels() &&;
  const vector<rx_byte>& data() const &;
  const vector<level>& levels() const &;
  const math::vec2z& dimensions() const &;
  pixel_format format() const;
  rx_size bpp() const;

private:
  void generate_mipchain(bool _has_mipchain, bool _want_mipchain);

  static pixel_format pixel_format_for_loader_bpp(rx_size _bpp);

  vector<rx_byte> m_data;
  vector<level> m_levels;
  math::vec2z m_dimensions;
  pixel_format m_pixel_format;
};

inline chain::pixel_format chain::pixel_format_for_loader_bpp(rx_size _bpp) {
  switch (_bpp) {
  case 4:
    return pixel_format::k_rgba_u8;
  case 3:
    return pixel_format::k_rgb_u8;
  case 1:
    return pixel_format::k_r_u8;
  }

  RX_HINT_UNREACHABLE();
}

inline chain::chain(memory::allocator* _allocator)
  : m_data{_allocator}
  , m_levels{_allocator}
{
}

inline chain::chain(chain&& _chain)
  : m_data{utility::move(_chain.m_data)}
  , m_levels{utility::move(_chain.m_levels)}
  , m_dimensions{_chain.m_dimensions}
  , m_pixel_format{_chain.m_pixel_format}
{
  _chain.m_dimensions = {};
}

inline chain& chain::operator=(chain&& _chain) {
  m_data = utility::move(_chain.m_data);
  m_levels = utility::move(_chain.m_levels);
  m_dimensions = _chain.m_dimensions;
  m_pixel_format = _chain.m_pixel_format;

  _chain.m_dimensions = {};

  return *this;
}

inline void chain::generate(loader&& _loader, bool _has_mipchain,
  bool _want_mipchain)
{
  generate(utility::move(_loader).data(),
    pixel_format_for_loader_bpp(_loader.bpp()), _loader.dimensions(),
    _has_mipchain, _want_mipchain);
}

inline vector<rx_byte>&& chain::data() && {
  return utility::move(m_data);
}

inline vector<chain::level>&& chain::levels() && {
  return utility::move(m_levels);
}

inline const vector<rx_byte>& chain::data() const & {
  return m_data;
}

inline const vector<chain::level>& chain::levels() const & {
  return m_levels;
}

inline const math::vec2z& chain::dimensions() const & {
  return m_dimensions;
}

inline chain::pixel_format chain::format() const {
  return m_pixel_format;
}

inline rx_size chain::bpp() const {
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
