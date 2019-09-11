#ifndef RX_MODEL_IMPORTER_H
#define RX_MODEL_IMPORTER_H
#include "rx/model/mesh.h"

#include "rx/core/concepts/interface.h"
#include "rx/core/vector.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/mat3x4.h"

namespace rx::model {

struct importer
  : concepts::interface
{
  importer(memory::allocator* _allocator);

  bool load(const string& _file_name);

  // implemented by each model loader
  virtual bool read(const vector<rx_byte>& _data) = 0;

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

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

  const vector<mesh>& meshes() const &;
  vector<mesh>&& meshes() &&;
  const vector<rx_u32>& elements() const &;
  vector<rx_u32>&& elements() &&;

  const vector<math::vec3f>& positions() const &;
  const vector<math::vec2f>& coordinates() const &;
  const vector<math::vec3f>& normals() const &;
  const vector<math::vec4f>& tangents() const &;

  // for skeletally animated models
  const vector<math::mat3x4f>& frames() const;

  vector<math::mat3x4f>&& frames();
  const vector<animation>& animations() const;
  vector<animation>&& animations();

  const vector<math::vec4b>& blend_indices() const &;
  const vector<math::vec4b>& blend_weights() const &;

  vector<joint>&& joints();
  const vector<joint>& joints() const;

protected:
  void generate_normals();
  bool generate_tangents();

  memory::allocator* m_allocator;

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

  string m_error;
};

template<typename... Ts>
inline bool importer::error(const char* _format, Ts&&... _arguments) {
  m_error = string::format(_format, utility::forward<Ts>(_arguments)...);
  return false;
}

inline const vector<mesh>& importer::meshes() const & {
  return m_meshes;
}

inline vector<mesh>&& importer::meshes() && {
  return utility::move(m_meshes);
}

inline const vector<rx_u32>& importer::elements() const & {
  return m_elements;
}

inline vector<rx_u32>&& importer::elements() && {
  return utility::move(m_elements);
}

inline const vector<math::vec3f>& importer::positions() const & {
  return m_positions;
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

inline const vector<math::mat3x4f>& importer::frames() const {
  return m_frames;
}

inline vector<math::mat3x4f>&& importer::frames() {
  return utility::move(m_frames);
}

inline const vector<importer::animation>& importer::animations() const {
  return m_animations;
}

inline vector<importer::animation>&& importer::animations() {
  return utility::move(m_animations);
}

inline const vector<math::vec4b>& importer::blend_indices() const & {
  return m_blend_indices;
}

inline const vector<math::vec4b>& importer::blend_weights() const & {
  return m_blend_weights;
}

inline vector<importer::joint>&& importer::joints() {
  return utility::move(m_joints);
}

inline const vector<importer::joint>& importer::joints() const {
  return m_joints;
}

} // namespace rx::model

#endif // RX_MODEL_IMPORTER_H