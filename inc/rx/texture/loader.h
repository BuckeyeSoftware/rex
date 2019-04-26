#ifndef RX_TEXTURE_LOADER_H
#define RX_TEXTURE_LOADER_H

#include <rx/core/array.h>
#include <rx/math/vec2.h>

namespace rx::texture {

struct loader {
  constexpr loader();
  constexpr loader(memory::allocator* _allocator);
  ~loader() = default;

  bool load(const char* _file_name);

  rx_size bpp() const;
  rx_size channels() const;
  const math::vec2z& dimensions() const &;
  array<rx_byte>&& data() &&;

private:
  memory::allocator* m_allocator;
  array<rx_byte> m_data;
  rx_size m_bpp;
  rx_size m_channels;
  math::vec2z m_dimensions;
};

inline constexpr loader::loader()
  : loader{&memory::g_system_allocator}
{
}

inline constexpr loader::loader(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_data{m_allocator}
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

inline array<rx_byte>&& loader::data() && {
  return utility::move(m_data);
}

} // namespace rx::texture

#endif // RX_TEXTURE_LOADER_H