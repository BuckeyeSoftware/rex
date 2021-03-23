#ifndef RX_MODEL_LOADER_H
#define RX_MODEL_LOADER_H
#include "rx/model/importer.h"

#include "rx/material/loader.h"

namespace Rx::Model {

struct Loader {
  RX_MARK_NO_COPY(Loader);
  RX_MARK_NO_MOVE(Loader);

  Loader(Memory::Allocator& _allocator);
  ~Loader();

  [[nodiscard]] bool load(Stream& _stream);
  [[nodiscard]] bool load(const String& _file_name);

  [[nodiscard]] bool parse(const JSON& _json);

  struct Vertex {
    Math::Vec3f position;
    Math::Vec3f normal;
    Math::Vec4f tangent;
    Math::Vec2f coordinate;
  };

  struct AnimatedVertex {
    Math::Vec3f position;
    Math::Vec3f normal;
    Math::Vec4f tangent;
    Math::Vec2f coordinate;
    Math::Vec4f blend_weights;
    Math::Vec4i blend_indices;
  };

  bool is_animated() const;

  const Vector<Vertex>& vertices() const;
  const Vector<Mesh>& meshes() const;
  const Vector<Uint32>& elements() const;
  const Map<String, Material::Loader>& materials() const;
  const Optional<Skeleton>& skeleton() const;

  // Only valid for animated models.
  const Vector<AnimatedVertex>& animated_vertices() const;
  const Vector<Clip>& clips() const;

  const String& name() const;

  constexpr Memory::Allocator& allocator() const;

  bool import(const String& _file_name);

private:
  void destroy();

  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  RX_HINT_FORMAT(3, 0)
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  bool parse_transform(const JSON& _transform);

  enum {
    CONSTRUCTED = 1 << 0,
    ANIMATED    = 1 << 1
  };

  Memory::Allocator& m_allocator;

  union {
    struct {} as_nat;
    Vector<Vertex> as_vertices;
    Vector<AnimatedVertex> as_animated_vertices;
  };

  Vector<Uint32> m_elements;
  Vector<Mesh> m_meshes;
  Vector<Clip> m_clips;
  Optional<Skeleton> m_skeleton;
  Vector<Math::Vec3f> m_positions;
  Optional<Math::Transform> m_transform;
  Map<String, Material::Loader> m_materials;
  String m_name;
  int m_flags;
};

template<typename... Ts>
bool Loader::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
void Loader::log(Log::Level _level, const char* _format,
  Ts&&... _arguments) const
{
  write_log(_level, String::format(_format, Utility::forward<Ts>(_arguments)...));
}

inline bool Loader::is_animated() const {
  return m_flags & ANIMATED;
}

inline const Vector<Loader::Vertex>& Loader::vertices() const {
  RX_ASSERT(!is_animated(), "not a static model");
  return as_vertices;
}

inline const Vector<Mesh>& Loader::meshes() const {
  return m_meshes;
}

inline const Vector<Uint32>& Loader::elements() const {
  return m_elements;
}

inline const Map<String, Material::Loader>& Loader::materials() const {
  return m_materials;
}

inline const Optional<Skeleton>& Loader::skeleton() const {
  return m_skeleton;
}

inline const Vector<Loader::AnimatedVertex>& Loader::animated_vertices() const {
  RX_ASSERT(is_animated(), "not a animated model");
  return as_animated_vertices;
}

inline const Vector<Clip>& Loader::clips() const {
  return m_clips;
}

inline const String& Loader::name() const {
  return m_name;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Loader::allocator() const {
  return m_allocator;
}

} // namespace Rx::Model

#endif // RX_MODEL_LOADER_H
