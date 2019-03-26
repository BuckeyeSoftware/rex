#ifndef RX_MODEL_LOADER_H
#define RX_MODEL_LOADER_H

#include <rx/core/concepts/interface.h>
#include <rx/core/array.h>
#include <rx/core/string.h>

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

  struct mesh {
    rx_size offset;
    rx_size count;
    string material;
  };

  const array<mesh>& meshes() const &;

  const array<rx_u32>& elements() const &;

  const array<math::vec3f>& positions() const &;
  const array<math::vec2f>& coordinates() const &;
  const array<math::vec3f>& normals() const &;
  const array<math::vec4f>& tangents() const &;

  // for skeletally animated models
  const array<math::vec4b>& blend_indices() const &;
  const array<math::vec4b>& blend_weights() const &;
  const array<math::mat3x4f>& frames() const &;
  const array<animation>& animations() const &;

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

  string m_error;
};

template<typename... Ts>
inline bool loader::error(const char* _format, Ts&&... _arguments) {
  m_error = string::format(_format, utility::forward<Ts>(_arguments)...);
  return false;
}

inline const array<loader::mesh>& loader::meshes() const & {
  return m_meshes;
}

inline const array<rx_u32>& loader::elements() const & {
  return m_elements;
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

inline const array<math::vec4b>& loader::blend_indices() const & {
  return m_blend_indices;
}

inline const array<math::vec4b>& loader::blend_weights() const & {
  return m_blend_weights;
}

inline const array<math::mat3x4f>& loader::frames() const & {
  return m_frames;
}

inline const array<loader::animation>& loader::animations() const & {
  return m_animations;
}

#if 0
struct animation_state {
  math::vec2z indices;
  rx_f32 offset;
  bool finished;
};

static animation_state animate(const loader::animation& _animation, rx_f32 _current_frame, bool _loop, rx_f32 _delta_time) {
  _current_frame += _animation.frame_rate;

  const bool finished{_current_frame >= _animation.frame_count - 1};
  const bool complete{finished && !_loop};

  _current_frame = math::mod(_current_frame, static_cast<rx_f32>(_animation.frame_count));

  rx_size frame1{complete ? _animation.frame_count - 1 : static_cast<rx_size>(_current_frame)};
  rx_size frame2{complete ? _animation.frame_count - 1 : frame1 + 1};

  frame1 %= _animation.frame_count;
  frame2 %= _animation.frame_count;

  animation_state state;
  state.indices[0] = _animation.frame_offset + frame1;
  state.indices[1] = _animation.frame_offset + frame2;
  state.offset = complete ? 0.0f : math::abs(_current_frame - static_cast<rx_f32>(frame1));
  state.finished = finished;

  return state;
}
#endif

} // namespace rx::model

#endif // RX_MODEL_LOADER_H