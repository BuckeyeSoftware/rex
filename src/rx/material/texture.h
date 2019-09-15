#ifndef RX_MATERIAL_TEXTURE_H
#define RX_MATERIAL_TEXTURE_H
#include "rx/core/log.h"
#include "rx/math/vec2.h"

#include "rx/texture/chain.h"

namespace rx {
  struct json;
}

namespace rx::material {

struct texture 
  : concepts::no_copy
{
  struct filter_options {
    bool bilinear;
    bool trilinear;
    bool mipmaps;
  };

  enum class wrap_type {
    k_clamp_to_edge,
    k_clamp_to_border,
    k_mirrored_repeat,
    k_repeat,
    k_mirror_clamp_to_edge
  };

  using wrap_options = math::vec2<wrap_type>;

  texture(memory::allocator* _allocator);
  texture(texture&& _texture);

  texture& operator=(texture&& _texture);

  bool load(const string& _file_name);
  bool parse(const json& _definition);

  const filter_options& filter() const &;
  const wrap_options& wrap() const &;
  const string& type() const &;

  rx::texture::chain&& chain();

private:
  bool parse_type(const json& _type);
  bool parse_filter(const json& _filter, bool _mipmaps);
  bool parse_wrap(const json& _wrap);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& _message) const;

  memory::allocator* m_allocator;
  rx::texture::chain m_chain;
  filter_options m_filter;
  wrap_options m_wrap;
  string m_type;
};

template<typename... Ts>
inline bool texture::error(const char* _format, Ts&&... _arguments) const {
  log(log::level::k_error, _format, utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
inline void texture::log(log::level _level, const char* _format, Ts&&... _arguments) const {
  write_log(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}

inline texture::texture(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_chain{_allocator}
{
}

inline texture::texture(texture&& _texture)
  : m_allocator{_texture.m_allocator}
  , m_chain{utility::move(_texture.m_chain)}
  , m_filter{_texture.m_filter}
  , m_wrap{_texture.m_wrap}
  , m_type{utility::move(_texture.m_type)}
{
}

inline texture& texture::operator=(texture&& _texture) {
  m_allocator = _texture.m_allocator;
  m_chain = utility::move(_texture.m_chain);
  m_filter = _texture.m_filter;
  m_wrap = _texture.m_wrap;
  m_type = utility::move(_texture.m_type);
  return *this;
}

inline const texture::filter_options& texture::filter() const & {
  return m_filter;
}

inline const texture::wrap_options& texture::wrap() const & {
  return m_wrap;
}

inline const string& texture::type() const & {
  return m_type;
}

inline rx::texture::chain&& texture::chain() {
  return utility::move(m_chain);
}

} // namespace rx::material

#endif // RX_MATERIAL_TEXTURE_H