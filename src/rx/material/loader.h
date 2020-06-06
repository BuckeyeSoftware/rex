#ifndef RX_MATERIAL_LOADER_H
#define RX_MATERIAL_LOADER_H
#include "rx/core/vector.h"
#include "rx/core/string.h"
#include "rx/core/map.h"
#include "rx/core/log.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/math/transform.h"

#include "rx/material/texture.h"

namespace Rx {
  struct JSON;
  struct Stream;
}

namespace Rx::Material {

struct RX_HINT_EMPTY_BASES Loader
  : Concepts::NoCopy
{
  Loader(Memory::Allocator& _allocator);
  Loader(Loader&& loader_);

  void operator=(Loader&& loader_);

  bool load(Stream* _stream);
  bool load(const String& _file_name);

  bool parse(const JSON& _definition);

  constexpr Memory::Allocator& allocator() const;
  Vector<Texture>&& textures();
  String&& name();
  bool alpha_test() const;
  bool has_alpha() const;
  bool no_compress() const;
  Float32 roughness() const;
  Float32 metalness() const;
  const Optional<Math::Transform>& transform() const &;

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  bool parse_textures(const JSON& _textures);

  enum {
    k_alpha_test  = 1 << 0,
    k_has_alpha   = 1 << 1,
    k_no_compress = 1 << 2
  };

  Ref<Memory::Allocator> m_allocator;
  Vector<Texture> m_textures;
  String m_name;
  Uint32 m_flags;
  Float32 m_roughness;
  Float32 m_metalness;
  Optional<Math::Transform> m_transform;
};

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Loader::allocator() const {
  return m_allocator;
}

inline Vector<Texture>&& Loader::textures() {
  return Utility::move(m_textures);
}

inline String&& Loader::name() {
  return Utility::move(m_name);
}

inline bool Loader::alpha_test() const {
  return m_flags & k_alpha_test;
}

inline bool Loader::has_alpha() const {
  return m_flags & k_has_alpha;
}

inline bool Loader::no_compress() const {
  return m_flags & k_no_compress;
}

inline Float32 Loader::roughness() const {
  return m_roughness;
}

inline Float32 Loader::metalness() const {
  return m_metalness;
}

inline const Optional<Math::Transform>& Loader::transform() const & {
  return m_transform;
}

template<typename... Ts>
inline bool Loader::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::k_error, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
inline void Loader::log(Log::Level _level, const char* _format,
  Ts&&... _arguments) const
{
  write_log(_level, String::format(_format, Utility::forward<Ts>(_arguments)...));
}

} // namespace rx::material

#endif // RX_MATERIAL_LOADER_H
