#ifndef RX_TEXTURE_LOADER_H
#define RX_TEXTURE_LOADER_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

#include "rx/core/hints/unreachable.h"

#include "rx/math/vec2.h"

namespace rx {
  struct stream;
}

namespace rx::texture {

enum class pixel_format {
  k_rgba_u8,
  k_bgra_u8,
  k_rgb_u8,
  k_bgr_u8,
  k_r_u8
};

struct loader
  : concepts::no_copy
  , concepts::no_move
{
  constexpr loader();
  constexpr loader(memory::allocator& _allocator);
  ~loader() = default;

  bool load(stream* _stream, pixel_format _want_format,
    const math::vec2z& _max_dimensions);
  bool load(stream* _stream, pixel_format _want_format);
  bool load(const string& _file_name, pixel_format _want_format);
  bool load(const string& _file_name, pixel_format _want_format,
    const math::vec2z& _max_dimensions);

  rx_size bpp() const;
  rx_size channels() const;
  const math::vec2z& dimensions() const &;
  vector<rx_byte>&& data();
  pixel_format format() const;

  constexpr memory::allocator& allocator() const;

private:
  memory::allocator& m_allocator;
  vector<rx_byte> m_data;
  rx_size m_bpp;
  rx_size m_channels;
  math::vec2z m_dimensions;
};

inline constexpr loader::loader()
  : loader{memory::system_allocator::instance()}
{
}

inline constexpr loader::loader(memory::allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{allocator()}
  , m_bpp{0}
  , m_channels{0}
  , m_dimensions{}
{
}

inline rx_size loader::bpp() const {
  return m_bpp;
}

inline rx_size loader::channels() const {
  return m_channels;
}

inline const math::vec2z& loader::dimensions() const & {
  return m_dimensions;
}

inline vector<rx_byte>&& loader::data() {
  return utility::move(m_data);
}

inline pixel_format loader::format() const {
  switch (m_bpp) {
  case 4:
    return pixel_format::k_rgba_u8;
  case 3:
    return pixel_format::k_rgb_u8;
  case 1:
    return pixel_format::k_r_u8;
  }
  RX_HINT_UNREACHABLE();
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& loader::allocator() const {
  return m_allocator;
}

} // namespace rx::texture

#endif // RX_TEXTURE_LOADER_H
