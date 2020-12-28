#ifndef RX_RENDER_PROGRAM_H
#define RX_RENDER_PROGRAM_H
#include "rx/core/string.h"
#include "rx/core/vector.h"
#include "rx/core/map.h"

#include "rx/math/vec2.h"
#include "rx/math/mat3x3.h"
#include "rx/math/mat3x4.h"
#include "rx/math/mat4x4.h"

#include "rx/render/frontend/resource.h"

namespace Rx::Render::Frontend {

struct Context;
struct Program;

struct Uniform {
  RX_MARK_NO_COPY(Uniform);

  enum class Type {
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

  Uniform();
  Uniform(Program* _program, Uint64 _bit, const String& _name, Type _type);
  Uniform(Uniform&& uniform_);
  ~Uniform();

  void record_sampler(int _sampler);
  void record_int(int _value);
  void record_vec2i(const Math::Vec2i& _value);
  void record_vec3i(const Math::Vec3i& _value);
  void record_vec4i(const Math::Vec4i& _value);
  void record_bool(bool _value);
  void record_float(Float32 _value);
  void record_vec2f(const Math::Vec2f& _value);
  void record_vec3f(const Math::Vec3f& _value);
  void record_vec4f(const Math::Vec4f& _value);
  void record_mat3x3f(const Math::Mat3x3f& _value);
  void record_mat4x4f(const Math::Mat4x4f& _value);
  void record_bones(const Vector<Math::Mat3x4f>& _frames, Size _joints);
  void record_raw(const Byte* _data, Size _size);

  Type type() const;
  const Byte* data() const;
  const String& name() const;
  Size size() const;
  bool is_padding() const;

  void flush(Byte* _data);

private:
  static Size size_for_type(Type _type);

  Program* m_program;
  Uint64 m_bit;
  Type m_type;
  union {
    Byte* as_opaque;
    Sint32* as_int;
    bool* as_boolean;
    Float32* as_float;
  };
  String m_name;
};

struct Shader {
  enum class Type {
    k_vertex,
    k_fragment
  };

  enum class InOutType {
    k_mat4x4f,
    k_mat3x3f,
    k_vec2i,
    k_vec3i,
    k_vec4i,
    k_vec2f,
    k_vec3f,
    k_vec4f,
    k_vec4b,
    k_float
  };

  Type kind;
  String source;

  struct InOut {
    Size index;
    InOutType kind;
  };

  Map<String, InOut> inputs;
  Map<String, InOut> outputs;
};

struct Program : Resource {
  Program(Context* _frontend);

  void validate() const;

  void add_shader(Shader&& shader_);
  Uniform& add_uniform(const String& _name, Uniform::Type _type, bool _is_padding);
  Uint64 dirty_uniforms_bitset() const;
  Size dirty_uniforms_size() const;

  void flush_dirty_uniforms(Byte* _data);

  const Vector<Uniform>& uniforms() const &;
  const Vector<Shader>& shaders() const &;
  Vector<Uniform>& uniforms() &;

private:
  String format_shader(const String& _source);

  void update_resource_usage();

  friend struct Uniform;

  void mark_uniform_dirty(Uint64 _uniform_index);

  Vector<Uniform> m_uniforms;
  Vector<Shader> m_shaders;
  Uint64 m_dirty_uniforms;
  Uint64 m_padding_uniforms;
};

// uniform
inline Uniform::Type Uniform::type() const {
  return m_type;
}

inline const Byte* Uniform::data() const {
  return as_opaque;
}

inline const String& Uniform::name() const {
  return m_name;
}

inline Size Uniform::size() const {
  return size_for_type(m_type);
}

inline bool Uniform::is_padding() const {
  return !!(m_program->m_padding_uniforms & m_bit);
}

// program
inline const Vector<Uniform>& Program::uniforms() const & {
  return m_uniforms;
}

inline const Vector<Shader>& Program::shaders() const & {
  return m_shaders;
}

inline Vector<Uniform>& Program::uniforms() & {
  return m_uniforms;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_PROGRAM_H
