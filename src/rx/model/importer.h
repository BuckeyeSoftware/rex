#ifndef RX_MODEL_IMPORTER_H
#define RX_MODEL_IMPORTER_H
#include "rx/core/vector.h"
#include "rx/core/log.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/aabb.h"
#include "rx/math/mat3x4.h"

namespace Rx {
struct Stream;
} // namespace rx

namespace Rx::Model {

struct Mesh {
  Size offset;
  Size count;
  String material;
  Math::AABB bounds;
};

struct Importer {
  RX_MARK_NO_COPY(Importer);
  RX_MARK_NO_MOVE(Importer);

  virtual ~Importer() = default;

  Importer(Memory::Allocator& _allocator);

  bool load(Stream* _stream);
  bool load(const String& _file_name);

  // implemented by each model loader
  virtual bool read(Stream* _stream) = 0;

  struct Animation {
    Float32 frame_rate;
    Size frame_offset;
    Size frame_count;
    String name;
  };

  struct Joint {
    Math::Mat3x4f frame;
    Sint32 parent;
  };

  Vector<Mesh>&& meshes();
  Vector<Uint32>&& elements();
  Vector<Math::Vec3f>&& positions();
  Vector<Joint>&& joints();

  const Vector<Math::Vec2f>& coordinates() const &;
  const Vector<Math::Vec3f>& normals() const &;
  const Vector<Math::Vec4f>& tangents() const &;

  // for skeletally animated models
  Vector<Math::Mat3x4f>&& frames();
  Vector<Animation>&& animations();
  const Vector<Math::Vec4b>& blend_indices() const &;
  const Vector<Math::Vec4b>& blend_weights() const &;

  constexpr Memory::Allocator& allocator() const;

protected:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  void generate_normals();
  bool generate_tangents();

  Memory::Allocator& m_allocator;

  Vector<Mesh> m_meshes;
  Vector<Uint32> m_elements;
  Vector<Math::Vec3f> m_positions;
  Vector<Math::Vec2f> m_coordinates;
  Vector<Math::Vec3f> m_normals;
  Vector<Math::Vec4f> m_tangents; // w = bitangent sign
  Vector<Math::Vec4b> m_blend_indices;
  Vector<Math::Vec4b> m_blend_weights;
  Vector<Math::Mat3x4f> m_frames;
  Vector<Animation> m_animations;
  Vector<Joint> m_joints;
  String m_name;
};

template<typename... Ts>
inline bool Importer::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
inline void Importer::log(Log::Level _level, const char* _format,
  Ts&&... _arguments) const
{
  write_log(_level, String::format(_format, Utility::forward<Ts>(_arguments)...));
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

inline Vector<Importer::Joint>&& Importer::joints() {
  return Utility::move(m_joints);
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

inline Vector<Math::Mat3x4f>&& Importer::frames() {
  return Utility::move(m_frames);
}

inline Vector<Importer::Animation>&& Importer::animations() {
  return Utility::move(m_animations);
}

inline const Vector<Math::Vec4b>& Importer::blend_indices() const & {
  return Utility::move(m_blend_indices);
}

inline const Vector<Math::Vec4b>& Importer::blend_weights() const & {
  return Utility::move(m_blend_weights);
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Importer::allocator() const {
  return m_allocator;
}

} // namespace Rx::Model

#endif // RX_MODEL_IMPORTER_H
