#ifndef RX_MODEL_LOADER_H
#define RX_MODEL_LOADER_H

#include <rx/model/mesh.h>

#include <rx/core/concepts/interface.h>
#include <rx/core/array.h>

#include <rx/math/vec2.h>
#include <rx/math/vec3.h>

#include <rx/math/mat3x4.h>

namespace rx::model {

struct loader : concepts::interface {
  loader(memory::allocator* _allocator);

  bool load(const string& _file_name);

  // implemented by each model loader
  virtual bool read(const array<rx_byte>& _data) = 0;

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  struct animation {
    rx_f32 frame_rate;
    rx_size frame_offset;
    rx_size frame_count;
    string name;
  };

  const array<mesh>& meshes() const &;
  array<mesh>&& meshes() &&;
  const array<rx_u32>& elements() const &;
  array<rx_u32>&& elements() &&;

  const array<math::vec3f>& positions() const &;
  const array<math::vec2f>& coordinates() const &;
  const array<math::vec3f>& normals() const &;
  const array<math::vec4f>& tangents() const &;

  // for skeletally animated models
  const array<math::mat3x4f>& frames() const &;
  array<math::mat3x4f>&& frames() &&;
  const array<animation>& animations() const &;
  array<animation>&& animations() &&;

  const array<math::vec4b>& blend_indices() const &;
  const array<math::vec4b>& blend_weights() const &;
  
  rx_size joints() const;

protected:
  void generate_normals();
  bool generate_tangents();

  memory::allocator* m_allocator;

  array<mesh> m_meshes;

  array<rx_u32> m_elements;

  array<math::vec3f> m_positions;
  array<math::vec2f> m_coordinates;
  array<math::vec3f> m_normals;
  array<math::vec4f> m_tangents; // w = bitangent sign

  array<math::vec4b> m_blend_indices;
  array<math::vec4b> m_blend_weights;
  array<math::mat3x4f> m_frames;
  array<animation> m_animations;
  rx_size m_joints;

  string m_error;
};

template<typename... Ts>
inline bool loader::error(const char* _format, Ts&&... _arguments) {
  m_error = string::format(_format, utility::forward<Ts>(_arguments)...);
  return false;
}

inline const array<mesh>& loader::meshes() const & {
  return m_meshes;
}

inline array<mesh>&& loader::meshes() && {
  return utility::move(m_meshes);
}

inline const array<rx_u32>& loader::elements() const & {
  return m_elements;
}

inline array<rx_u32>&& loader::elements() && {
  return utility::move(m_elements);
}

inline const array<math::vec3f>& loader::positions() const & {
  return m_positions;
}

inline const array<math::vec2f>& loader::coordinates() const & {
  return m_coordinates;
}

inline const array<math::vec3f>& loader::normals() const & {
  return m_normals;
}

inline const array<math::vec4f>& loader::tangents() const & {
  return m_tangents;
}

inline const array<math::mat3x4f>& loader::frames() const & {
  return m_frames;
}

inline array<math::mat3x4f>&& loader::frames() && {
  return utility::move(m_frames);
}

inline const array<loader::animation>& loader::animations() const & {
  return m_animations;
}

inline array<loader::animation>&& loader::animations() && {
  return utility::move(m_animations);
}

inline const array<math::vec4b>& loader::blend_indices() const & {
  return m_blend_indices;
}

inline const array<math::vec4b>& loader::blend_weights() const & {
  return m_blend_weights;
}

inline rx_size loader::joints() const {
  return m_joints;
}

} // namespace rx::model

#endif // RX_MODEL_LOADER_H