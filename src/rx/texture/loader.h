#ifndef RX_TEXTURE_LOADER_H
#define RX_TEXTURE_LOADER_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

#include "rx/math/vec2.h"

namespace rx::texture {

struct loader {
  constexpr loader();
  constexpr loader(memory::allocator* _allocator);
  ~loader() = default;

  bool load(const string& _file_name);

  rx_size bpp() const;
  rx_size channels() const;
  const math::vec2z& dimensions() const &;
  vector<rx_byte>&& data();
  bool has_alpha() const;

private:
  memory::allocator* m_allocator;
  vector<rx_byte> m_data;
  rx_size m_bpp;
  rx_size m_channels;
  math::vec2z m_dimensions;
  bool m_has_alpha;
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
  , m_has_alpha{false}
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

} // namespace rx::texture

#endif // RX_TEXTURE_LOADER_H