#ifndef RX_MODEL_IMPORTER_H
#define RX_MODEL_IMPORTER_H
#include "rx/core/concepts/interface.h"
#include "rx/core/vector.h"
#include "rx/core/log.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/aabb.h"
#include "rx/math/mat3x4.h"

namespace rx {
struct stream;
} // namespace rx

namespace rx::model {

struct mesh {
  rx_size offset;
  rx_size count;
  string material;
  math::aabb bounds;
};

struct importer
  : concepts::interface
{
  importer(memory::allocator& _allocator);

  bool load(stream* _stream);
  bool load(const string& _file_name);

  // implemented by each model loader
  virtual bool read(stream* _stream) = 0;

  struct animation {
    rx_f32 frame_rate;
    rx_size frame_offset;
    rx_size frame_count;
    string name;
  };

  struct joint {
    math::mat3x4f frame;
    rx_s32 parent;
  };

  vector<mesh>&& meshes();
  vector<rx_u32>&& elements();
  vector<math::vec3f>&& positions();
  vector<joint>&& joints();

  const vector<math::vec2f>& coordinates() const &;
  const vector<math::vec3f>& normals() const &;
  const vector<math::vec4f>& tangents() const &;

  // for skeletally animated models
  vector<math::mat3x4f>&& frames();
  vector<animation>&& animations();
  const vector<math::vec4b>& blend_indices() const &;
  const vector<math::vec4b>& blend_weights() const &;

  constexpr memory::allocator& allocator() const;

protected:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& message_) const;

  void generate_normals();
  bool generate_tangents();

  memory::allocator& m_allocator;

  vector<mesh> m_meshes;
  vector<rx_u32> m_elements;
  vector<math::vec3f> m_positions;
  vector<math::vec2f> m_coordinates;
  vector<math::vec3f> m_normals;
  vector<math::vec4f> m_tangents; // w = bitangent sign
  vector<math::vec4b> m_blend_indices;
  vector<math::vec4b> m_blend_weights;
  vector<math::mat3x4f> m_frames;
  vector<animation> m_animations;
  vector<joint> m_joints;
  string m_name;
};

template<typename... Ts>
inline bool importer::error(const char* _format, Ts&&... _arguments) const {
  log(log::level::k_error, _format, utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
inline void importer::log(log::level _level, const char* _format,
  Ts&&... _arguments) const
{
  write_log(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}

inline vector<mesh>&& importer::meshes() {
  return utility::move(m_meshes);
}

inline vector<rx_u32>&& importer::elements() {
  return utility::move(m_elements);
}

inline vector<math::vec3f>&& importer::positions() {
  return utility::move(m_positions);
}

inline vector<importer::joint>&& importer::joints() {
  return utility::move(m_joints);
}

inline const vector<math::vec2f>& importer::coordinates() const & {
  return m_coordinates;
}

inline const vector<math::vec3f>& importer::normals() const & {
  return m_normals;
}

inline const vector<math::vec4f>& importer::tangents() const & {
  return m_tangents;
}

inline vector<math::mat3x4f>&& importer::frames() {
  return utility::move(m_frames);
}

inline vector<importer::animation>&& importer::animations() {
  return utility::move(m_animations);
}

inline const vector<math::vec4b>& importer::blend_indices() const & {
  return utility::move(m_blend_indices);
}

inline const vector<math::vec4b>& importer::blend_weights() const & {
  return utility::move(m_blend_weights);
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& importer::allocator() const {
  return m_allocator;
}

} // namespace rx::model

#endif // RX_MODEL_IMPORTER_H
