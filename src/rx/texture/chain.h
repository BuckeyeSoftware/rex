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
  struct level {
    rx_size offset;
    rx_size size;
    math::vec2z dimensions;
  };

  constexpr chain();
  constexpr chain(memory::allocator* _allocator);
  chain(chain&& chain_);

  chain& operator=(chain&& chain_);

  void generate(loader&& loader_, bool _has_mipchain, bool _want_mipchain);
  void generate(loader&& loader_, pixel_format _want_format,
    bool _has_mipchain, bool _want_mipchain);
  void generate(vector<rx_byte>&& data_, pixel_format _has_format,
    pixel_format _want_format, const math::vec2z& _dimensions,
    bool _has_mipchain, bool _want_mipchain);
  void generate(const rx_byte* _data, pixel_format _has_format,
    pixel_format _want_format, const math::vec2z& _dimensions,
    bool _has_mipchain, bool _want_mipchain);

  void resize(const math::vec2z& _dimensions);

  vector<rx_byte>&& data();
  vector<level>&& levels();
  const vector<rx_byte>& data() const;
  const vector<level>& levels() const;
  const math::vec2z& dimensions() const;
  pixel_format format() const;
  rx_size bpp() const;

private:
  void generate_mipchain(bool _has_mipchain, bool _want_mipchain);

  memory::allocator* m_allocator;
  vector<rx_byte> m_data;
  vector<level> m_levels;
  math::vec2z m_dimensions;
  union {
    utility::nat m_nat;
    pixel_format m_pixel_format;
  };
};

inline constexpr chain::chain()
  : chain{&memory::g_system_allocator}
{
}

inline constexpr chain::chain(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_data{m_allocator}
  , m_levels{m_allocator}
  , m_nat{}
{
}

inline chain::chain(chain&& chain_)
  : m_allocator{chain_.m_allocator}
  , m_data{utility::move(chain_.m_data)}
  , m_levels{utility::move(chain_.m_levels)}
  , m_dimensions{chain_.m_dimensions}
  , m_pixel_format{chain_.m_pixel_format}
{
  chain_.m_allocator = nullptr;
  chain_.m_dimensions = {};
}

inline chain& chain::operator=(chain&& chain_) {
  RX_ASSERT(&chain_ != this, "self assignment");

  m_allocator = chain_.m_allocator;
  m_data = utility::move(chain_.m_data);
  m_levels = utility::move(chain_.m_levels);
  m_dimensions = chain_.m_dimensions;
  m_pixel_format = chain_.m_pixel_format;

  chain_.m_allocator = nullptr;
  chain_.m_dimensions = {};

  return *this;
}

inline void chain::generate(loader&& loader_, bool _has_mipchain,
  bool _want_mipchain)
{
  const pixel_format format{loader_.format()};
  generate(utility::move(loader_.data()), format, format, loader_.dimensions(),
    _has_mipchain, _want_mipchain);
}

inline void chain::generate(loader&& loader_, pixel_format _want_format,
  bool _has_mipchain, bool _want_mipchain)
{
  generate(utility::move(loader_.data()), loader_.format(), _want_format,
    loader_.dimensions(), _has_mipchain, _want_mipchain);
}

inline vector<rx_byte>&& chain::data() {
  return utility::move(m_data);
}

inline vector<chain::level>&& chain::levels() {
  return utility::move(m_levels);
}

inline const vector<rx_byte>& chain::data() const {
  return m_data;
}

inline const vector<chain::level>& chain::levels() const {
  return m_levels;
}

inline const math::vec2z& chain::dimensions() const {
  return m_dimensions;
}

inline pixel_format chain::format() const {
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
