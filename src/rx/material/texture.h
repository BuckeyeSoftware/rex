#ifndef RX_MATERIAL_TEXTURE_H
#define RX_MATERIAL_TEXTURE_H
#include "rx/core/log.h"
#include "rx/core/optional.h"

#include "rx/math/transform.h"

#include "rx/texture/chain.h"

namespace Rx {
  struct JSON;
  struct Stream;
} // namespace Rx

namespace Rx::Material {

struct Texture {
  RX_MARK_NO_COPY(Texture);

  struct Filter {
    bool bilinear;
    bool trilinear;
    bool mipmaps;
  };

  enum class WrapType : Byte {
    k_clamp_to_edge,
    k_clamp_to_border,
    k_mirrored_repeat,
    k_repeat,
    k_mirror_clamp_to_edge
  };

  using Wrap = Math::Vec2<WrapType>;

  Texture(Memory::Allocator& _allocator);
  Texture(Texture&& texture_);

  Texture& operator=(Texture&& texture_);

  bool load(Stream* _stream);
  bool load(const String& _file_name);

  bool parse(const JSON& _definition);

  const Filter& filter() const &;
  const Wrap& wrap() const &;
  const String& type() const &;
  const String& file() const &;
  const Optional<Math::Vec4f>& border() const &;

  const Rx::Texture::Chain& chain() const;
  Rx::Texture::Chain&& chain();

  constexpr Memory::Allocator& allocator() const;

private:
  bool load_texture_file(const Math::Vec2z& _max_dimensions);

  bool parse_type(const JSON& _type);
  bool parse_filter(const JSON& _filter, bool& _mipmaps);
  bool parse_wrap(const JSON& _wrap);
  bool parse_border(const JSON& _border);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  Memory::Allocator* m_allocator;
  Rx::Texture::Chain m_chain;
  Filter m_filter;
  Wrap m_wrap;
  String m_type;
  String m_file;
  Optional<Math::Vec4f> m_border;
};

template<typename... Ts>
bool Texture::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
void Texture::log(Log::Level _level, const char* _format, Ts&&... _arguments) const {
  write_log(_level, String::format(_format, Utility::forward<Ts>(_arguments)...));
}

inline Texture::Texture(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_chain{allocator()}
  , m_type{allocator()}
  , m_file{allocator()}
{
}

inline Texture::Texture(Texture&& texture_)
  : m_allocator{texture_.m_allocator}
  , m_chain{Utility::move(texture_.m_chain)}
  , m_filter{texture_.m_filter}
  , m_wrap{texture_.m_wrap}
  , m_type{Utility::move(texture_.m_type)}
  , m_file{Utility::move(texture_.m_file)}
  , m_border{Utility::move(texture_.m_border)}
{
}

inline Texture& Texture::operator=(Texture&& texture_) {
  RX_ASSERT(&texture_ != this, "self assignment");

  m_allocator = texture_.m_allocator;
  m_chain = Utility::move(texture_.m_chain);
  m_filter = texture_.m_filter;
  m_wrap = texture_.m_wrap;
  m_type = Utility::move(texture_.m_type);
  m_file = Utility::move(texture_.m_file);
  m_border = Utility::move(texture_.m_border);

  return *this;
}

inline const Texture::Filter& Texture::filter() const & {
  return m_filter;
}

inline const Texture::Wrap& Texture::wrap() const & {
  return m_wrap;
}

inline const String& Texture::type() const & {
  return m_type;
}

inline const String& Texture::file() const & {
  return m_file;
}

inline const Optional<Math::Vec4f>& Texture::border() const & {
  return m_border;
}

inline const Rx::Texture::Chain& Texture::chain() const {
  return m_chain;
}

inline Rx::Texture::Chain&& Texture::chain() {
  return Utility::move(m_chain);
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Texture::allocator() const {
  return *m_allocator;
}

} // namespace Rx::Material

#endif // RX_MATERIAL_TEXTURE_H
