#ifndef RX_MODEL_LOADER_H
#define RX_MODEL_LOADER_H
#include "rx/model/mesh.h"

#include "rx/core/concepts/interface.h"
#include "rx/core/vector.h"

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/mat3x4.h"

namespace rx::model {

struct loader : concepts::interface {
  loader(memory::allocator* _allocator);

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
inline bool loader::error(const char* _format, Ts&&... _arguments) {
  m_error = string::format(_format, utility::forward<Ts>(_arguments)...);
  return false;
}

inline const vector<mesh>& loader::meshes() const & {
  return m_meshes;
}

inline vector<mesh>&& loader::meshes() && {
  return utility::move(m_meshes);
}

inline const vector<rx_u32>& loader::elements() const & {
  return m_elements;
}

inline vector<rx_u32>&& loader::elements() && {
  return utility::move(m_elements);
}

inline const vector<math::vec3f>& loader::positions() const & {
  return m_positions;
}

inline const vector<math::vec2f>& loader::coordinates() const & {
  return m_coordinates;
}

inline const vector<math::vec3f>& loader::normals() const & {
  return m_normals;
}

inline const vector<math::vec4f>& loader::tangents() const & {
  return m_tangents;
}

inline const vector<math::mat3x4f>& loader::frames() const {
  return m_frames;
}

inline vector<math::mat3x4f>&& loader::frames() {
  return utility::move(m_frames);
}

inline const vector<loader::animation>& loader::animations() const {
  return m_animations;
}

inline vector<loader::animation>&& loader::animations() {
  return utility::move(m_animations);
}

inline const vector<math::vec4b>& loader::blend_indices() const & {
  return m_blend_indices;
}

inline const vector<math::vec4b>& loader::blend_weights() const & {
  return m_blend_weights;
}

inline vector<loader::joint>&& loader::joints() {
  return utility::move(m_joints);
}

inline const vector<loader::joint>& loader::joints() const {
  return m_joints;
}

} // namespace rx::model

#endif // RX_MODEL_LOADER_H