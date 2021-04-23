#ifndef RX_MODEL_IMPORTER_H
#define RX_MODEL_IMPORTER_H
#include "rx/core/vector.h"
#include "rx/core/log.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/aabb.h"
#include "rx/math/mat3x4.h"

#include "rx/model/skeleton.h"
#include "rx/model/animation.h"

namespace Rx::Stream { struct UntrackedStream; }

namespace Rx::Model {

struct Mesh {
  Size offset;
  Size count;
  String material;

  // Contains the per frame bounds for mesh. When the mesh contains no
  // animations, bounds[0][0] contains the bounds for the static mesh.
  Vector<Vector<Math::AABB>> bounds; // bounds[animation][frame]
};

struct Importer {
  RX_MARK_NO_COPY(Importer);
  RX_MARK_NO_MOVE(Importer);

  virtual ~Importer() = default;

  Importer(Memory::Allocator& _allocator);

  [[nodiscard]] bool load(Stream::UntrackedStream& _stream);
  [[nodiscard]] bool load(const String& _file_name);

  // implemented by each model loader
  [[nodiscard]] virtual bool read(Stream::UntrackedStream& _stream) = 0;

  Vector<Mesh>&& meshes();
  Vector<Uint32>&& elements();
  Vector<Math::Vec3f>&& positions();
  Optional<Skeleton>&& skeleton();

  const Vector<Math::Vec2f>& coordinates() const &;
  const Vector<Math::Vec3f>& normals() const &;
  const Vector<Math::Vec4f>& tangents() const &;

  // Only for skeletally animated models.
  Vector<Clip>&& clips();
  const Vector<Math::Vec4i>& blend_indices() const &;
  const Vector<Math::Vec4f>& blend_weights() const &;

  constexpr Memory::Allocator& allocator() const;

protected:
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  RX_HINT_FORMAT(3, 0)
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  [[nodiscard]] bool generate_normals();
  [[nodiscard]] bool generate_tangents();

  Memory::Allocator& m_allocator;

  Vector<Mesh> m_meshes;
  Vector<Uint32> m_elements;
  Vector<Math::Vec3f> m_positions;
  Vector<Math::Vec2f> m_coordinates;
  Vector<Math::Vec3f> m_normals;
  Vector<Math::Vec4f> m_tangents; // w = bitangent sign
  Vector<Math::Vec4i> m_blend_indices;
  Vector<Math::Vec4f> m_blend_weights;
  Vector<Clip> m_clips;
  Optional<Skeleton> m_skeleton;
  String m_name;
};

template<typename... Ts>
bool Importer::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
void Importer::log(Log::Level _level, const char* _format,
  Ts&&... _arguments) const
{
  if constexpr(sizeof...(Ts) > 0) {
    auto format = String::format(m_allocator, _format, Utility::forward<Ts>(_arguments)...);
    write_log(_level, Utility::move(format));
  } else {
    write_log(_level, _format);
  }
}

inline Vector<Mesh>&& Importer::meshes() {
  return Utility::move(m_meshes);
}

inline Vector<Uint32>&& Importer::elements() {
  return Utility::move(m_elements);
}

inline Vector<Math::Vec3f>&& Importer::positions() {
  return Utility::move(m_positions);
}

inline const Vector<Math::Vec2f>& Importer::coordinates() const & {
  return m_coordinates;
}

inline const Vector<Math::Vec3f>& Importer::normals() const & {
  return m_normals;
}

inline const Vector<Math::Vec4f>& Importer::tangents() const & {
  return m_tangents;
}

inline Optional<Skeleton>&& Importer::skeleton() {
  return Utility::move(m_skeleton);
}

inline Vector<Clip>&& Importer::clips() {
  return Utility::move(m_clips);
}

inline const Vector<Math::Vec4i>& Importer::blend_indices() const & {
  return Utility::move(m_blend_indices);
}

inline const Vector<Math::Vec4f>& Importer::blend_weights() const & {
  return Utility::move(m_blend_weights);
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Importer::allocator() const {
  return m_allocator;
}

} // namespace Rx::Model

#endif // RX_MODEL_IMPORTER_H
