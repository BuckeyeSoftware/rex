#ifndef RX_MATERIAL_LOADER_H
#define RX_MATERIAL_LOADER_H
#include "rx/core/vector.h"
#include "rx/core/map.h"

#include "rx/math/transform.h"

#include "rx/material/texture.h"

namespace Rx {
  struct JSON;
} // namespace Rx

namespace Rx::Stream {
  struct Context;
} // namespace Rx::Stream

namespace Rx::Material {

struct Loader {
  RX_MARK_NO_COPY(Loader);

  Loader(Memory::Allocator& _allocator);
  Loader(Loader&& loader_);

  Loader& operator=(Loader&& loader_);

  [[nodiscard]] bool load(Stream::Context& _stream);
  [[nodiscard]] bool load(const String& _file_name);

  [[nodiscard]] bool parse(const JSON& _definition);

  constexpr Memory::Allocator& allocator() const;
  const Vector<Texture>& textures() const;
  const String& name() const;
  bool alpha_test() const;
  bool no_compress() const;
  Float32 roughness() const;
  Float32 metalness() const;
  Float32 occlusion() const;
  const Math::Vec3f& emission() const &;
  const Math::Vec3f& albedo() const &;
  const Optional<Math::Transform>& transform() const &;

private:
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  RX_HINT_FORMAT(3, 0)
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  bool parse_textures(const JSON& _textures);

  enum {
    ALPHA_TEST  = 1 << 0,
    NO_COMPRESS = 1 << 1
  };

  Memory::Allocator* m_allocator;
  Vector<Texture> m_textures;
  String m_name;
  Uint32 m_flags;
  Float32 m_roughness;
  Float32 m_metalness;
  Float32 m_occlusion;
  Math::Vec3f m_albedo;
  Math::Vec3f m_emission;
  Optional<Math::Transform> m_transform;
};

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Loader::allocator() const {
  return *m_allocator;
}

inline const Vector<Texture>& Loader::textures() const {
  return m_textures;
}

inline const String& Loader::name() const {
  return m_name;
}

inline bool Loader::alpha_test() const {
  return m_flags & ALPHA_TEST;
}

inline bool Loader::no_compress() const {
  return m_flags & NO_COMPRESS;
}

inline Float32 Loader::roughness() const {
  return m_roughness;
}

inline Float32 Loader::metalness() const {
  return m_metalness;
}

inline Float32 Loader::occlusion() const {
  return m_occlusion;
}

inline const Math::Vec3f& Loader::emission() const & {
  return m_emission;
}

inline const Math::Vec3f& Loader::albedo() const & {
  return m_albedo;
}

inline const Optional<Math::Transform>& Loader::transform() const & {
  return m_transform;
}

template<typename... Ts>
bool Loader::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
void Loader::log(Log::Level _level, const char* _format,
  Ts&&... _arguments) const
{
  if constexpr(sizeof...(Ts) > 0) {
    auto format = String::format(allocator(), _format,
      Utility::forward<Ts>(_arguments)...);
    write_log(_level, Utility::move(format));
  } else {
    write_log(_level, _format);
  }
}

} // namespace Rx::Material

#endif // RX_MATERIAL_LOADER_H
