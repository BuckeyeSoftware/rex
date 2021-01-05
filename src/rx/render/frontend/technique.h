#ifndef RX_RENDER_FRONTEND_TECHNIQUE_H
#define RX_RENDER_FRONTEND_TECHNIQUE_H
#include "rx/core/log.h"

#include "rx/render/frontend/program.h"

namespace Rx {
  struct JSON;
  struct Stream;
}

namespace Rx::Render::Frontend {

struct Program;
struct Technique;
struct Context;
struct Module;

struct Technique {
  RX_MARK_NO_COPY(Technique);

  Technique() = default;
  Technique(Context* _frontend);
  ~Technique();

  Technique(Technique&& technique_);
  Technique& operator=(Technique&& technique_);

  enum class Type : Uint8 {
    BASIC,
    VARIANT,
    PERMUTE
  };

  Type type() const;
  bool has_variants() const;
  bool has_permutes() const;

  operator Program*() const;

  Program* permute(Uint64 _flags) const;
  Program* variant(Size _index) const;

  bool load(Stream* _stream);
  bool load(const String& _file_name);

  bool parse(const JSON& _description);
  bool compile(const Map<String, Module>& _modules);

  const String& name() const;

private:
  void fini();

  struct UniformDefinition {
    union Variant {
      constexpr Variant()
        : as_nat{}
      {
      }

      Utility::Nat as_nat;
      Sint32 as_int;
      Float32 as_float;
      bool as_bool;
      Math::Vec2i as_vec2i;
      Math::Vec3i as_vec3i;
      Math::Vec4i as_vec4i;
      Math::Vec2f as_vec2f;
      Math::Vec3f as_vec3f;
      Math::Vec4f as_vec4f;
      Math::Mat3x3f as_mat3x3f;
      Math::Mat4x4f as_mat4x4f;
    };

    Uniform::Type kind;
    String name;
    String when;
    Variant value;
    bool has_value;
  };

  struct ShaderDefinition {
    struct InOut {
      Shader::InOutType kind;
      Size index;
      String when;
    };

    Shader::Type kind;
    String source;
    Vector<String> dependencies;
    Map<String, InOut> inputs;
    Map<String, InOut> outputs;
    String when;
  };

  bool evaluate_when_for_permute(const String& _when, Uint64 _flags) const;
  bool evaluate_when_for_variant(const String& _when, Size _index) const;
  bool evaluate_when_for_basic(const String& _when) const;

  bool parse_uniforms(const JSON& _uniforms);
  bool parse_shaders(const JSON& _shaders);

  bool parse_uniform(const JSON& _uniform);
  bool parse_shader(const JSON& _shader);

  bool parse_inouts(const JSON& _inouts, const char* _type,
                    Map<String, ShaderDefinition::InOut>& inouts_);

  bool parse_inout(const JSON& _inout, const char* _type,
                   Map<String, ShaderDefinition::InOut>& inouts_,
                   Size& index_);

  bool parse_specializations(const JSON& _specializations, const char* _type);
  bool parse_specialization(const JSON& _specialization, const char* _type);

  bool resolve_dependencies(const Map<String, Module>& _modules);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  Context* m_frontend;
  Type m_type;
  Vector<Program*> m_programs;
  Vector<Uint64> m_permute_flags;
  String m_name;

  Vector<ShaderDefinition> m_shader_definitions;
  Vector<UniformDefinition> m_uniform_definitions;
  Vector<String> m_specializations;
};

inline const String& Technique::name() const {
  return m_name;
}

template<typename... Ts>
bool Technique::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
void Technique::log(Log::Level _level, const char* _format,  Ts&&... _arguments) const {
  if constexpr (sizeof...(Ts) != 0) {
    write_log(_level, String::format(_format, Utility::forward<Ts>(_arguments)...));
  } else {
    write_log(_level, _format);
  }
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_TECHNIQUE_H
