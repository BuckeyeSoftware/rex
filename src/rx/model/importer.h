#ifndef RX_MODEL_IMPORTER_H
#define RX_MODEL_IMPORTER_H
#include "rx/core/vector.h"
#include "rx/core/report.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/aabb.h"
#include "rx/math/mat3x4.h"

#include "rx/model/skeleton.h"
#include "rx/model/animation.h"

namespace Rx::Stream { struct Context; }

namespace Rx::Model {

struct Mesh {
  String name;
  String material;
  Size offset;
  Size count;

  // Contains the per frame bounds for mesh. When the mesh contains no
  // animations, bounds[0][0] contains the bounds for the static mesh.
  Vector<Vector<Math::AABB>> bounds; // bounds[animation][frame]
};

struct Importer {
  RX_MARK_NO_COPY(Importer);
  RX_MARK_NO_MOVE(Importer);

  virtual ~Importer() = default;

  Importer(Memory::Allocator& _allocator);

  [[nodiscard]] bool load(Stream::Context& _stream);
  [[nodiscard]] bool load(const StringView& _file_name);

  // implemented by each model loader
  [[nodiscard]] virtual bool read(Stream::Context& _stream) = 0;

  Vector<Mesh>&& meshes();
  Vector<Uint32>&& elements();
  Optional<Skeleton>&& skeleton();

  const Vector<Math::Vec3f>& positions() const &;
  const Vector<Float32>& occlusions() const &;
  const Vector<Math::Vec2f>& coordinates() const &;
  const Vector<Math::Vec3f>& normals() const &;
  const Vector<Math::Vec4f>& tangents() const &;

  // Only for skeletally animated models.
  Vector<Clip>&& clips();
  const Vector<Math::Vec4i>& blend_indices() const &;
  const Vector<Math::Vec4f>& blend_weights() const &;

  constexpr Memory::Allocator& allocator() const;

protected:
  [[nodiscard]] bool generate_normals();
  [[nodiscard]] bool generate_tangents();

  Memory::Allocator& m_allocator;

  Vector<Mesh> m_meshes;
  Vector<Uint32> m_elements;
  Vector<Math::Vec3f> m_positions;
  Vector<Float32> m_occlusions;
  Vector<Math::Vec2f> m_coordinates;
  Vector<Math::Vec3f> m_normals;
  Vector<Math::Vec4f> m_tangents; // w = bitangent sign
  Vector<Math::Vec4i> m_blend_indices;
  Vector<Math::Vec4f> m_blend_weights;
  Vector<Clip> m_clips;
  Optional<Skeleton> m_skeleton;
  String m_name;
  Report m_report;
};

inline Vector<Mesh>&& Importer::meshes() {
  return Utility::move(m_meshes);
}

inline Vector<Uint32>&& Importer::elements() {
  return Utility::move(m_elements);
}

inline const Vector<Math::Vec3f>& Importer::positions() const & {
  return m_positions;
}

inline const Vector<Float32>& Importer::occlusions() const & {
  return m_occlusions;
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
