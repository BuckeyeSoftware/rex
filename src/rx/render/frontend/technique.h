#ifndef RX_RENDER_FRONTEND_TECHNIQUE_H
#define RX_RENDER_FRONTEND_TECHNIQUE_H
#include "rx/core/report.h"

#include "rx/render/frontend/program.h"

namespace Rx { struct JSON; }
namespace Rx::Stream { struct UntrackedStream; }

namespace Rx::Render::Frontend {

struct Program;
struct Technique;
struct Context;
struct Module;

struct Technique {
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
      Math::Mat3x4f as_mat3x4f;
      Math::Mat4x4f as_mat4x4f;
    };

    Uniform::Type kind;
    String name;
    String when;
    Optional<Variant> value;
  };

public:
  RX_MARK_NO_COPY(Technique);

  Technique() = delete;
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

    Configuration(Technique* _technique, Type _type, String&& name_);
    Configuration(Configuration&& configuration_);
    ~Configuration();

    const String& name() const &;

    Program* basic() const;
    Program* permute(Uint64 _flags) const;
    Program* variant(Size _index) const;

  private:
    friend struct Technique;

    bool parse_specializations(const JSON& _specializations, const char* _type);
    bool parse_specialization(const JSON& _specialization, const char* _type);

    bool compile(const Map<String, Module>& _modules,
      const Map<String, bool>& _values);

    void release();

    // Lazily evaluated program. Only compiles when needed.
    //
    // Can call |compile| to force evaluation though.
    struct LazyProgram {
      LazyProgram(Technique* _technique);
      LazyProgram(LazyProgram&& intermediate_);
      ~LazyProgram();

      bool add_shader(Shader&& shader_);
      bool add_uniform(const UniformDefinition& _uniform_definition, bool _is_padding);

      bool compile();

      Program* evaluate() const;

    private:
      friend Technique;

      Technique* m_technique;
      Vector<Shader> m_shaders;
      Vector<Pair<UniformDefinition, bool>> m_uniforms;
      mutable Program* m_program;
    };

    Technique* m_technique;
    Type m_type;
    String m_name;
    Vector<LazyProgram> m_programs;
    Vector<Uint64> m_permute_flags;
    Vector<String> m_specializations;
  };

  [[nodiscard]] bool load(Stream::UntrackedStream& _stream);
  [[nodiscard]] bool load(const String& _file_name);

  [[nodiscard]] bool parse(const JSON& _description);
  [[nodiscard]] bool compile(const Map<String, Module>& _modules);

  const Configuration& configuration(Size _index) const;

  const String& name() const;

private:
  struct ShaderDefinition {
    ShaderDefinition(Memory::Allocator& _allocator)
      : source{_allocator}
      , dependencies{_allocator}
      , inputs{_allocator}
      , outputs{_allocator}
      , when{_allocator}
    {
    }

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

  Context* m_frontend;

  String m_name;

  Vector<Configuration> m_configurations;
  Vector<ShaderDefinition> m_shader_definitions;
  Vector<UniformDefinition> m_uniform_definitions;
  Report m_report;
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
  , m_report{Utility::move(technique_.m_report)}
{
  // Update |m_configuration| references to this Technique instance.
  m_configurations.each_fwd([this](Configuration& configuration_) {
    configuration_.m_technique = this;
    // Update |m_programs| references to this Technique instance.
    configuration_.m_programs.each_fwd([this](Configuration::LazyProgram& program_) {
      program_.m_technique = this;
    });
  });
}

inline const Technique::Configuration& Technique::configuration(Size _index) const {
  return m_configurations[_index];
}

inline const String& Technique::name() const {
  return m_name;
}

// [Technique::Configuration]
inline Technique::Configuration::Configuration(Configuration&& configuration_)
  : m_technique{Utility::exchange(configuration_.m_technique, nullptr)}
  , m_type{Utility::exchange(configuration_.m_type, Type::BASIC)}
  , m_name{Utility::move(configuration_.m_name)}
  , m_programs{Utility::move(configuration_.m_programs)}
  , m_permute_flags{Utility::move(configuration_.m_permute_flags)}
  , m_specializations{Utility::move(configuration_.m_specializations)}
{
}

inline Technique::Configuration::~Configuration() {
  release();
}

inline const String& Technique::Configuration::name() const & {
  return m_name;
}

// [Technique::Configuration::LazyProgram]
inline Technique::Configuration::LazyProgram::LazyProgram(LazyProgram&& program_)
  : m_technique{Utility::exchange(program_.m_technique, nullptr)}
  , m_shaders{Utility::move(program_.m_shaders)}
  , m_uniforms{Utility::move(program_.m_uniforms)}
  , m_program{Utility::exchange(program_.m_program, nullptr)}
{
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_TECHNIQUE_H
