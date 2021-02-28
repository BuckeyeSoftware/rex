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

  struct Configuration {
    RX_MARK_NO_COPY(Configuration);

    enum class Type : Uint8 {
      BASIC,
      VARIANT,
      PERMUTE
    };

    Configuration(Technique* _technique, Type _type);
    Configuration(Configuration&& configuration_);
    ~Configuration();
    Configuration& operator=(Configuration&& configguration_);

    const String& name() const &;

    Program* basic() const;
    Program* permute(Uint64 _flags) const;
    Program* variant(Size _index) const;

  private:
    friend struct Technique;

    bool parse_specializations(const JSON& _specializations, const char* _type);
    bool parse_specialization(const JSON& _specialization, const char* _type);

    bool compile(const Map<String, Module>& _modules);

    void release();

    Technique* m_technique;
    Type m_type;
    String m_name;
    Vector<Program*> m_programs;
    Vector<Uint64> m_permute_flags;
    Vector<String> m_specializations;
  };

  [[nodiscard]] bool load(Stream* _stream);
  [[nodiscard]] bool load(const String& _file_name);

  [[nodiscard]] bool parse(const JSON& _description);
  [[nodiscard]] bool compile(const Map<String, Module>& _modules);

  const Configuration& configuration(Size _index) const;

  const String& name() const;

private:
  struct UniformDefinition {
    union Variant {
      constexpr Variant()
        : as_nat{}
      {
      }

      struct {} as_nat;
      Sint32 as_int;
      Float32 as_float;
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

    struct Dependency {
      String name;
      String when;
    };

    Shader::Type kind;
    String source;
    Vector<Dependency> dependencies;
    Map<String, InOut> inputs;
    Map<String, InOut> outputs;
    String when;
  };

  Optional<String> resolve_source(
    const ShaderDefinition& _definition,
    const Map<String, bool>& _values,
    const Map<String, Module>& _modules) const;

  bool evaluate_when(const Map<String, bool>& _values, const String& _when) const;

  bool parse_uniforms(const JSON& _uniforms);
  bool parse_shaders(const JSON& _shaders);
  bool parse_configurations(const JSON& _configurations);

  bool parse_uniform(const JSON& _uniform);
  bool parse_shader(const JSON& _shader);
  bool parse_configuration(const JSON& _configuration);

  bool parse_inouts(const JSON& _inouts, const char* _type,
                    Map<String, ShaderDefinition::InOut>& inouts_);

  bool parse_inout(const JSON& _inout, const char* _type,
                   Map<String, ShaderDefinition::InOut>& inouts_,
                   Size& index_);

  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  RX_HINT_FORMAT(3, 0)
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  Context* m_frontend;

  String m_name;

  Vector<Configuration> m_configurations;
  Vector<ShaderDefinition> m_shader_definitions;
  Vector<UniformDefinition> m_uniform_definitions;
};

// [Technique]
inline Technique::~Technique() {
  m_configurations.clear();
}

inline Technique::Technique(Technique&& technique_)
  : m_frontend{Utility::exchange(technique_.m_frontend, nullptr)}
  , m_name{Utility::move(technique_.m_name)}
  , m_configurations{Utility::move(technique_.m_configurations)}
  , m_shader_definitions{Utility::move(technique_.m_shader_definitions)}
  , m_uniform_definitions{Utility::move(technique_.m_uniform_definitions)}
{
  // Update |m_configuration| references to this Technique instance.
  m_configurations.each_fwd([this](Configuration& configuration_) {
    configuration_.m_technique = this;
  });
}

inline const Technique::Configuration& Technique::configuration(Size _index) const {
  return m_configurations[_index];
}

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

// [Technique::Configuration]
inline Technique::Configuration::Configuration(Configuration&& configuration_)
  : m_technique{Utility::exchange(configuration_.m_technique, nullptr)}
  , m_type{Utility::exchange(configuration_.m_type, Type::BASIC)}
  , m_programs{Utility::move(configuration_.m_programs)}
  , m_permute_flags{Utility::move(configuration_.m_permute_flags)}
  , m_specializations{Utility::move(configuration_.m_specializations)}
{
}

inline Technique::Configuration::~Configuration() {
  release();
}

inline Technique::Configuration& Technique::Configuration::operator=(Configuration&& configuration_) {
  if (this != &configuration_) {
    release();
    m_technique = Utility::exchange(configuration_.m_technique, nullptr);
    m_type = Utility::exchange(configuration_.m_type, Type::BASIC);
    m_programs = Utility::move(configuration_.m_programs);
    m_permute_flags = Utility::move(configuration_.m_permute_flags);
    m_specializations = Utility::move(configuration_.m_specializations);
  }
  return *this;
}

inline const String& Technique::Configuration::name() const & {
  return m_name;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_TECHNIQUE_H
