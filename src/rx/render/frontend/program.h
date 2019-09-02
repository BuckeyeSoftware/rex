#ifndef RX_RENDER_PROGRAM_H
#define RX_RENDER_PROGRAM_H
#include "rx/core/string.h"
#include "rx/core/vector.h"
#include "rx/core/map.h"

#include "rx/math/vec2.h"
#include "rx/math/mat3x3.h"
#include "rx/math/mat3x4.h"
#include "rx/math/mat4x4.h"

#include "rx/core/concepts/no_copy.h"

#include "rx/render/frontend/resource.h"

namespace rx::render::frontend {

struct interface;
struct program;

struct uniform
  : concepts::no_copy
{
  enum class type {
    k_sampler1D,
    k_sampler2D,
    k_sampler3D,
    k_samplerCM,
    k_bool,
    k_int,
    k_float,
    k_vec2i,
    k_vec3i,
    k_vec4i,
    k_vec2f,
    k_vec3f,
    k_vec4f,
    k_mat4x4f,
    k_mat3x3f,
    k_bonesf
  };

  uniform();
  uniform(program* _program, rx_size _index, const string& _name, type _type);
  uniform(uniform&& _uniform);
  ~uniform();

  void record_sampler(int _sampler);
  void record_int(int _value);
  void record_vec2i(const math::vec2i& _value);
  void record_vec3i(const math::vec3i& _value);
  void record_vec4i(const math::vec4i& _value);
  void record_bool(bool _value);
  void record_float(rx_f32 _value);
  void record_vec2f(const math::vec2f& _value);
  void record_vec3f(const math::vec3f& _value);
  void record_vec4f(const math::vec4f& _value);
  void record_mat3x3f(const math::mat3x3f& _value);
  void record_mat4x4f(const math::mat4x4f& _value);
  void record_bones(const vector<math::mat3x4f>& _frames, rx_size _joints);
  void record_raw(const rx_byte* _data, rx_size _size);

  type kind() const;
  const rx_byte* data() const;
  const string& name() const;
  rx_size size() const;

  void flush(rx_byte* _data);

private:
  static rx_size size_for_type(type _type);

  program* m_program;
  rx_u64 m_mask;
  type m_type;
  union {
    rx_byte* as_opaque;
    rx_s32* as_int;
    bool* as_boolean;
    rx_f32* as_float;
  };
  string m_name;
};

inline uniform::type uniform::kind() const {
  return m_type;
}

inline const rx_byte* uniform::data() const {
  return as_opaque;
}

inline const string& uniform::name() const {
  return m_name;
}

inline rx_size uniform::size() const {
  return size_for_type(m_type);
}

struct shader {
  enum class type {
    k_vertex,
    k_fragment
  };

  enum class inout_type {
    k_vec2i,
    k_vec3i,
    k_vec4i,
    k_vec2f,
    k_vec3f,
    k_vec4f,
    k_vec4b,
    k_float
  };

  type kind;
  string source;

  struct inout {
    rx_size index;
    inout_type kind;
  };

  map<string, inout> inputs;
  map<string, inout> outputs;
};

struct program : resource {
  program(interface* _frontend);

  void validate() const;

  void add_shader(shader&& _shader);
  uniform& add_uniform(const string& _name, uniform::type _type);
  rx_u64 dirty_uniforms_bitset() const;
  rx_size dirty_uniforms_size() const;

  void flush_dirty_uniforms(rx_byte* _data);

  const vector<uniform>& uniforms() const &;
  const vector<shader>& shaders() const &;
  vector<uniform>& uniforms() &;

private:
  void update_resource_usage();

  friend struct uniform;

  memory::allocator* m_allocator;
  vector<uniform> m_uniforms;
  vector<shader> m_shaders;
  rx_u64 m_dirty_uniforms;
};

inline const vector<uniform>& program::uniforms() const & {
  return m_uniforms;
}

inline const vector<shader>& program::shaders() const & {
  return m_shaders;
}

inline vector<uniform>& program::uniforms() & {
  return m_uniforms;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_PROGRAM_H