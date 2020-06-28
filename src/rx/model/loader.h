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

  bool load(Stream* _stream);
  bool load(const String& _file_name);

  bool parse(const JSON& _json);

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
    Math::Vec4b blend_weights;
    Math::Vec4b blend_indices;
  };

  bool is_animated() const;

  Vector<Vertex>&& vertices();
  Vector<Mesh>&& meshes();
  Vector<Uint32>&& elements();
  Map<String, Material::Loader>&& materials();

  // Only valid for animated models.
  Vector<AnimatedVertex>&& animated_vertices();
  Vector<Importer::Joint>&& joints();
  const Vector<Importer::Animation>& animations() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  friend struct Animation;

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  bool import(const String& _file_name);
  bool parse_transform(const JSON& _transform);

  enum {
    k_constructed = 1 << 0,
    k_animated    = 1 << 1
  };

  Memory::Allocator& m_allocator;

  union {
    Utility::Nat as_nat;
    Vector<Vertex> as_vertices;
    Vector<AnimatedVertex> as_animated_vertices;
  };

  Vector<Uint32> m_elements;
  Vector<Mesh> m_meshes;
  Vector<Importer::Animation> m_animations;
  Vector<Importer::Joint> m_joints;
  Vector<Math::Vec3f> m_positions;
  Vector<Math::Mat3x4f> m_frames;
  Optional<Math::Transform> m_transform;
  Map<String, Material::Loader> m_materials;
  String m_name;
  int m_flags;
};

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

inline bool Loader::is_animated() const {
  return m_flags & k_animated;
}

inline Vector<Loader::Vertex>&& Loader::vertices() {
  RX_ASSERT(!is_animated(), "not a static model");
  return Utility::move(as_vertices);
}

inline Vector<Mesh>&& Loader::meshes() {
  return Utility::move(m_meshes);
}

inline Vector<Uint32>&& Loader::elements() {
  return Utility::move(m_elements);
}

inline Map<String, Material::Loader>&& Loader::materials() {
  return Utility::move(m_materials);
}

inline Vector<Loader::AnimatedVertex>&& Loader::animated_vertices() {
  RX_ASSERT(is_animated(), "not a animated model");
  return Utility::move(as_animated_vertices);
}

inline Vector<Importer::Joint>&& Loader::joints() {
  RX_ASSERT(is_animated(), "not a animated model");
  return Utility::move(m_joints);
}

inline const Vector<Importer::Animation>& Loader::animations() const & {
  return m_animations;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Loader::allocator() const {
  return m_allocator;
}

} // namespace rx::model

#endif // RX_MODEL_LOADER_H
