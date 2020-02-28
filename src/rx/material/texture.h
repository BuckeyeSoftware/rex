#ifndef RX_MATERIAL_TEXTURE_H
#define RX_MATERIAL_TEXTURE_H
#include "rx/core/log.h"
#include "rx/core/optional.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/math/transform.h"

#include "rx/texture/chain.h"

namespace rx {
  struct json;
  struct stream;
} // namespace rx

namespace rx::material {

struct RX_HINT_EMPTY_BASES texture
  : concepts::no_copy
{
  struct filter_options {
    bool bilinear;
    bool trilinear;
    bool mipmaps;
  };

  enum class wrap_type : rx_byte {
    k_clamp_to_edge,
    k_clamp_to_border,
    k_mirrored_repeat,
    k_repeat,
    k_mirror_clamp_to_edge
  };

  using wrap_options = math::vec2<wrap_type>;

  texture(memory::allocator* _allocator);
  texture(texture&& texture_);

  texture& operator=(texture&& texture_);

  bool load(stream* _stream);
  bool load(const string& _file_name);

  bool parse(const json& _definition);

  const filter_options& filter() const &;
  const wrap_options& wrap() const &;
  const string& type() const &;
  const string& file() const &;
  const optional<math::vec4f>& border() const &;

  const rx::texture::chain& chain() const;
  rx::texture::chain&& chain();

private:
  bool parse_type(const json& _type);
  bool parse_filter(const json& _filter, bool& _mipmaps);
  bool parse_wrap(const json& _wrap);
  bool parse_border(const json& _border);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& message_) const;

  memory::allocator* m_allocator;
  rx::texture::chain m_chain;
  filter_options m_filter;
  wrap_options m_wrap;
  string m_type;
  string m_file;
  optional<math::vec4f> m_border;
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
  , m_chain{m_allocator}
  , m_type{m_allocator}
  , m_file{m_allocator}
{
}

inline texture::texture(texture&& texture_)
  : m_allocator{texture_.m_allocator}
  , m_chain{utility::move(texture_.m_chain)}
  , m_filter{texture_.m_filter}
  , m_wrap{texture_.m_wrap}
  , m_type{utility::move(texture_.m_type)}
  , m_file{utility::move(texture_.m_file)}
  , m_border{utility::move(texture_.m_border)}
{
}

inline texture& texture::operator=(texture&& texture_) {
  RX_ASSERT(&texture_ != this, "self assignment");

  m_allocator = texture_.m_allocator;
  m_chain = utility::move(texture_.m_chain);
  m_filter = texture_.m_filter;
  m_wrap = texture_.m_wrap;
  m_type = utility::move(texture_.m_type);
  m_file = utility::move(texture_.m_file);
  m_border = utility::move(texture_.m_border);

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

inline const string& texture::file() const & {
  return m_file;
}

inline const optional<math::vec4f>& texture::border() const & {
  return m_border;
}

inline const rx::texture::chain& texture::chain() const {
  return m_chain;
}

inline rx::texture::chain&& texture::chain() {
  return utility::move(m_chain);
}

} // namespace rx::material

#endif // RX_MATERIAL_TEXTURE_H
