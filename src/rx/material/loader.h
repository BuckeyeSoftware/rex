#ifndef RX_MATERIAL_LOADER_H
#define RX_MATERIAL_LOADER_H
#include "rx/core/vector.h"
#include "rx/core/map.h"
#include "rx/core/report.h"

#include "rx/math/transform.h"

#include "rx/material/texture.h"

namespace Rx::Serialize { struct JSON; }
namespace Rx::Stream { struct Context; }

namespace Rx::Material {

struct Loader {
  RX_MARK_NO_COPY(Loader);

  Loader(Memory::Allocator& _allocator);
  Loader(Loader&& loader_);

  Loader& operator=(Loader&& loader_);

  [[nodiscard]] bool load(Stream::Context& _stream);
  [[nodiscard]] bool load(const StringView& _file_name);

  [[nodiscard]] bool parse(const Serialize::JSON& _definition);

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
  bool parse_textures(const Serialize::JSON& _textures);

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
  Report m_report;
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

} // namespace Rx::Material

#endif // RX_MATERIAL_LOADER_H
