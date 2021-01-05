#include <string.h> // strncmp

#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/module.h"

#include "rx/core/json.h"
#include "rx/core/optional.h"
#include "rx/core/filesystem/file.h"
#include "rx/core/algorithm/topological_sort.h"

RX_LOG("render/technique", logger);

namespace Rx::Render::Frontend {

// simple recursive descent parser and evaluator for declarative predicates
//
// letter     = "A" | "B" | "C" | "D" | "E" | "F" | "G"
//            | "H" | "I" | "J" | "K" | "L" | "M" | "N"
//            | "O" | "P" | "Q" | "R" | "S" | "T" | "U"
//            | "V" | "W" | "X" | "Y" | "Z" | "a" | "b"
//            | "c" | "d" | "e" | "f" | "g" | "h" | "i"
//            | "j" | "k" | "l" | "m" | "n" | "o" | "p"
//            | "q" | "r" | "s" | "t" | "u" | "v" | "w"
//            | "x" | "y" | "z" ;
// digit      = "0" | "1" | "2" | "3" | "4" | "5" | "6"
//            | "7" | "8" | "9" ;
// identifier = letter , { letter | digit | "_" } ;
// expression = expression, "&&", value
//            | expression, "||", value
//            | value ;
// value      = element
//            | "!", element ;
// element    = "(", expression, ")"
//            | identifier ;
enum {
  UNMATCHED_PARENTHESIS        = -1,
  UNEXPECTED_CHARACTER         = -2,
  UNEXPECTED_END_OF_EXPRESSION = -3,
  UNDECLARED_IDENTIFIER        = -4
};

static constexpr const char* binexp_result_to_string(int _result) {
  switch (_result) {
  case UNMATCHED_PARENTHESIS:
    return "unmatched parenthesis";
  case UNEXPECTED_CHARACTER:
    return "unexpected character";
  case UNEXPECTED_END_OF_EXPRESSION:
    return "unexpected end of expression";
  case UNDECLARED_IDENTIFIER:
    return "undeclared identifier";
  }
  return nullptr;
}

static void binexp_skip_spaces(const char*& expression_) {
  for (; *expression_ == ' '; expression_++);
}

static int binexp_parse_binary(const char*& expression_, int& parenthesis_, const Map<String, bool>& _values);
static int binexp_parse_atom(const char*& expression_, int& parenthesis_, const Map<String, bool>& _values) {
  binexp_skip_spaces(expression_);

  bool negated{false};
  if (*expression_ == '!') {
    negated = true;
    expression_++; // skip '!'
  }

  if (*expression_ == '(') {
    expression_++; // skip '('
    parenthesis_++;
    const auto result{binexp_parse_binary(expression_, parenthesis_, _values)};
    if (result < 0) {
      return result;
    }
    if (*expression_ != ')') {
      return UNMATCHED_PARENTHESIS;
    }
    expression_++; // skip ')'
    parenthesis_--;
    return negated ? !result : result;
  }

  String identifier;
  const char* end = strpbrk(expression_, " )"); // ' ' or ')' marks end of an identifier
  if (end) {
    identifier = {expression_, end};
  } else {
    // otherwise the identifier is at the end of the string
    identifier = expression_;
  }

  const auto find = _values.find(identifier);
  if (!find) {
    return UNDECLARED_IDENTIFIER;
  }

  expression_ += identifier.size();

  return negated ? !*find : *find;
}

static int binexp_parse_binary(const char*& expression_, int& parenthesis_, const Map<String, bool>& _values) {
  auto result{binexp_parse_atom(expression_, parenthesis_, _values)};
  if (result < 0) {
    return result;
  }

  for (;;) {
    binexp_skip_spaces(expression_);
    const bool is_and = !strncmp(expression_, "&&", 2);
    const bool is_or = !strncmp(expression_, "||", 2);
    if (!is_and && !is_or) {
      return result;
    }

    expression_ += 2; // skip '&&' or '||'
    const auto tail_result = binexp_parse_atom(expression_, parenthesis_, _values);
    if (tail_result < 0) {
      return tail_result;
    }

    if (is_and) {
      result &= tail_result;
    } else {
      result |= tail_result;
    }
  }

  return UNEXPECTED_END_OF_EXPRESSION;
}

static int binexp_evaluate(const char* _expression, const Map<String, bool>& _values) {
  if (!*_expression) {
    return 1;
  }

  int parenthesis = 0;
  const auto result = binexp_parse_binary(_expression, parenthesis, _values);
  if (result < 0) {
    return result;
  }
  if (parenthesis != 0 || *_expression == ')') {
    return UNMATCHED_PARENTHESIS;
  }
  if (*_expression != '\0') {
    return UNEXPECTED_CHARACTER;
  }
  return result;
}

static Optional<Uniform::Type> uniform_type_from_string(const String& _type) {
  static constexpr const struct {
    const char* match;
    Uniform::Type kind;
  } TABLE[] = {
    {"sampler1D", Uniform::Type::SAMPLER1D},
    {"sampler2D", Uniform::Type::SAMPLER2D},
    {"sampler3D", Uniform::Type::SAMPLER3D},
    {"samplerCM", Uniform::Type::SAMPLERCM},
    {"bool",      Uniform::Type::BOOL},
    {"int",       Uniform::Type::INT},
    {"float",     Uniform::Type::FLOAT},
    {"vec2i",     Uniform::Type::VEC2I},
    {"vec3i",     Uniform::Type::VEC3I},
    {"vec4i",     Uniform::Type::VEC4I},
    {"vec2f",     Uniform::Type::VEC2F},
    {"vec3f",     Uniform::Type::VEC3F},
    {"vec4f",     Uniform::Type::VEC4F},
    {"mat4x4f",   Uniform::Type::MAT4X4F},
    {"mat3x3f",   Uniform::Type::MAT3X3F},
    {"bonesf",    Uniform::Type::BONES}
  };

  for (const auto& element : TABLE) {
    if (_type == element.match) {
      return element.kind;
    }
  }

  return nullopt;
}

static Optional<Shader::InOutType> inout_type_from_string(const String& _type) {
  static constexpr const struct {
    const char* match;
    Shader::InOutType kind;
  } TABLE[] = {
    {"mat4x4f", Shader::InOutType::MAT4X4F},
    {"mat3x3f", Shader::InOutType::MAT3X3F},
    {"vec2i",   Shader::InOutType::VEC2I},
    {"vec3i",   Shader::InOutType::VEC3I},
    {"vec4i",   Shader::InOutType::VEC4I},
    {"vec2f",   Shader::InOutType::VEC2F},
    {"vec3f",   Shader::InOutType::VEC3F},
    {"vec4f",   Shader::InOutType::VEC4F},
    {"vec4b",   Shader::InOutType::VEC4B},
    {"float",   Shader::InOutType::FLOAT}
  };

  for (const auto& element : TABLE) {
    if (element.match == _type) {
      return element.kind;
    }
  }

  return nullopt;
}

Technique::Technique(Context* _frontend)
  : m_frontend{_frontend}
  , m_programs{m_frontend->allocator()}
  , m_permute_flags{m_frontend->allocator()}
  , m_name{m_frontend->allocator()}
  , m_shader_definitions{m_frontend->allocator()}
  , m_uniform_definitions{m_frontend->allocator()}
  , m_specializations{m_frontend->allocator()}
{
}

Technique::~Technique() {
  fini();
}

Technique::Technique(Technique&& technique_)
  : m_frontend{Utility::exchange(technique_.m_frontend, nullptr)}
  , m_type{technique_.m_type}
  , m_programs{Utility::move(technique_.m_programs)}
  , m_permute_flags{Utility::move(technique_.m_permute_flags)}
  , m_name{Utility::move(technique_.m_name)}
  , m_shader_definitions{Utility::move(technique_.m_shader_definitions)}
  , m_uniform_definitions{Utility::move(technique_.m_uniform_definitions)}
  , m_specializations{Utility::move(technique_.m_specializations)}
{
}

Technique& Technique::operator=(Technique&& technique_) {
  RX_ASSERT(&technique_ != this, "self assignment");

  fini();

  m_frontend = Utility::exchange(technique_.m_frontend, nullptr);
  m_type = technique_.m_type;
  m_programs = Utility::move(technique_.m_programs);
  m_name = Utility::move(technique_.m_name);
  m_shader_definitions = Utility::move(technique_.m_shader_definitions);
  m_uniform_definitions = Utility::move(technique_.m_uniform_definitions);
  m_specializations = Utility::move(technique_.m_specializations);

  return *this;
}

bool Technique::evaluate_when_for_permute(const String& _when, Uint64 _flags) const {
  Map<String, bool> values{m_frontend->allocator()};
  for (Size i = 0; i < m_specializations.size(); i++) {
    values.insert(m_specializations[i], _flags & (1_u64 << i));
  }
  const int result{binexp_evaluate(_when.data(), values)};
  if (result < 0) {
    return error("when expression evaluation failed: %s for \"%s\"",
      binexp_result_to_string(result), _when);
  }
  return result;
}

bool Technique::evaluate_when_for_variant(const String& _when, Size _index) const {
  Map<String, bool> values{m_frontend->allocator()};
  for (Size i = 0; i < m_specializations.size(); i++) {
    values.insert(m_specializations[i], i == _index);
  }
  const int result{binexp_evaluate(_when.data(), values)};
  if (result < 0) {
    return error("when expression evaluation failed: %s for \"%s\"",
      binexp_result_to_string(result), _when);
  }
  return result;
}

bool Technique::evaluate_when_for_basic(const String& _when) const {
  return _when.is_empty();
}

bool Technique::compile(const Map<String, Module>& _modules) {
  // Resolve each shaders dependencies.
  if (!resolve_dependencies(_modules)) {
    return false;
  }

  ShaderDefinition* vertex{nullptr};
  ShaderDefinition* fragment{nullptr};
  m_shader_definitions.each_fwd([&](ShaderDefinition& _shader) {
    if (_shader.kind == Shader::Type::FRAGMENT) {
      fragment = &_shader;
    } else if (_shader.kind == Shader::Type::VERTEX) {
      vertex = &_shader;
    }
    return true;
  });

  // if we have a fragment shader, ensure we have a vertex shader to go with it
  if (fragment) {
    if (!vertex) {
      return error("missing vertex shader");
    }

    // ensure all fragment inputs wire correctly into vertex outputs
    const bool check_inouts{
      // enumerate all vertex outputs and check for matching fragment inputs
      vertex->outputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout_definition) {
        const auto check{fragment->inputs.find(_name)};
        if (!check) {
          return error("could not find fragment input for vertex output '%s'", _name);
        }
        if (check->kind != _inout_definition.kind) {
          return error("Type mismatch for fragment input '%s'", _name);
        }
        if (check->when != _inout_definition.when) {
          return error("when mismatch for fragment input '%s'", _name);
        }
        return true;
      })
      &&
      // enumerate all fragment inputs and check for matching vertex outputs
      fragment->inputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout_definition) {
        const auto check{vertex->outputs.find(_name)};
        if (!check) {
          return error("could not find vertex output for fragment input '%s'", _name);
        }
        if (check->kind != _inout_definition.kind) {
          return error("Type mismatch for vertex output '%s'", _name);
        }
        if (check->when != _inout_definition.when) {
          return error("when mismatch for vertex output '%s'", _name);
        }
        return true;
      })
    };

    if (!check_inouts) {
      return false;
    }

    // ensure there is at least one fragment output
    if (fragment->outputs.is_empty()) {
      return error("missing output in fragment shader");
    }
  }

  if (m_type == Type::BASIC) {
    // create and add just a single program to m_programs
    auto program{m_frontend->create_program(RX_RENDER_TAG("technique"))};

    m_shader_definitions.each_fwd([&](const ShaderDefinition& _shader_definition) {
      if (evaluate_when_for_basic(_shader_definition.when)) {
        Shader specialized_shader;
        specialized_shader.kind = _shader_definition.kind;
        specialized_shader.source = _shader_definition.source;

        // emit inputs
        _shader_definition.inputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout) {
          if (evaluate_when_for_basic(_inout.when)) {
            specialized_shader.inputs.insert(_name, {_inout.index, _inout.kind});
          }
        });

        // emit outputs
        _shader_definition.outputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout){
          if (evaluate_when_for_basic(_inout.when)) {
            specialized_shader.outputs.insert(_name, {_inout.index, _inout.kind});
          }
        });

        program->add_shader(Utility::move(specialized_shader));
      }
    });

    m_uniform_definitions.each_fwd([&](const UniformDefinition& _uniform_definition) {
      auto& uniform{program->add_uniform(_uniform_definition.name, _uniform_definition.kind,
        !evaluate_when_for_basic(_uniform_definition.when))};
      if (_uniform_definition.has_value) {
        const auto* data{reinterpret_cast<const Byte*>(&_uniform_definition.value)};
        uniform.record_raw(data, uniform.size());
      }
    });

    m_frontend->initialize_program(RX_RENDER_TAG("technique"), program);

    m_programs.push_back(program);
  } else if (m_type == Type::PERMUTE) {
    const Uint64 mask{(1_u64 << m_specializations.size()) - 1};
    auto generate{[&](Uint64 _flags) {
      m_permute_flags.push_back(_flags);

      auto program{m_frontend->create_program(RX_RENDER_TAG("technique"))};

      m_shader_definitions.each_fwd([&](const ShaderDefinition& _shader_definition) {
        if (evaluate_when_for_permute(_shader_definition.when, _flags)) {
          Shader specialized_shader;
          specialized_shader.kind = _shader_definition.kind;

          // emit #defines
          const Size specializations{m_specializations.size()};
          for (Size i{0}; i < specializations; i++) {
            const String& specialication{m_specializations[i]};
            if (_flags & (1_u64 << i)) {
              specialized_shader.source.append(String::format("#define %s\n", specialication));
            }
          }

          // append shader source
          specialized_shader.source.append(_shader_definition.source);

          // emit inputs
          _shader_definition.inputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout) {
            if (evaluate_when_for_permute(_inout.when, _flags)) {
              specialized_shader.inputs.insert(_name, {_inout.index, _inout.kind});
            }
          });

          // emit outputs
          _shader_definition.outputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout) {
            if (evaluate_when_for_permute(_inout.when, _flags)) {
              specialized_shader.outputs.insert(_name, {_inout.index, _inout.kind});
            }
          });

          program->add_shader(Utility::move(specialized_shader));
        }
      });

      // emit uniforms
      m_uniform_definitions.each_fwd([&](const UniformDefinition& _uniform_definition) {
        auto& uniform{program->add_uniform(_uniform_definition.name, _uniform_definition.kind,
          !evaluate_when_for_permute(_uniform_definition.when, _flags))};
        if (_uniform_definition.has_value) {
          const auto* data{reinterpret_cast<const Byte*>(&_uniform_definition.value)};
          uniform.record_raw(data, uniform.size());
        }
      });

      // initialize and track
      m_frontend->initialize_program(RX_RENDER_TAG("technique"), program);
      m_programs.push_back(program);
    }};

    for (Uint64 flags{0}; flags != mask; flags = ((flags | ~mask) + 1_u64) & mask) {
      generate(flags);
    }

    generate(mask);
  } else if (m_type == Type::VARIANT) {
    const Size specializations{m_specializations.size()};
    for (Size i{0}; i < specializations; i++) {
      const auto& specialization{m_specializations[i]};
      auto program{m_frontend->create_program(RX_RENDER_TAG("technique"))};
      m_shader_definitions.each_fwd([&](const ShaderDefinition& _shader_definition) {
        if (evaluate_when_for_variant(_shader_definition.when, i)) {
          Shader specialized_shader;
          specialized_shader.kind = _shader_definition.kind;

          // emit #defines
          specialized_shader.source.append(String::format("#define %s\n", specialization));

          // append shader source
          specialized_shader.source.append(_shader_definition.source);

          // emit inputs
          _shader_definition.inputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout) {
            if (evaluate_when_for_variant(_inout.when, i)) {
              specialized_shader.inputs.insert(_name, {_inout.index, _inout.kind});
            }
          });

          // emit outputs
          _shader_definition.outputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout){
            if (evaluate_when_for_variant(_inout.when, i)) {
              specialized_shader.outputs.insert(_name, {_inout.index, _inout.kind});
            }
          });

          program->add_shader(Utility::move(specialized_shader));
        }
      });

      // emit uniforms
      m_uniform_definitions.each_fwd([&](const UniformDefinition& _uniform_definition) {
        auto& uniform{program->add_uniform(_uniform_definition.name, _uniform_definition.kind,
          !evaluate_when_for_variant(_uniform_definition.when, i))};
        if (_uniform_definition.has_value) {
          const auto* data{reinterpret_cast<const Byte*>(&_uniform_definition.value)};
          uniform.record_raw(data, uniform.size());
        }
      });

      // initialize and track
      m_frontend->initialize_program(RX_RENDER_TAG("technique"), program);
      m_programs.push_back(program);
    };
  }

  return true;
}

Technique::operator Program*() const {
  RX_ASSERT(m_type == Type::BASIC, "not a basic technique");
  return m_programs[0];
}

Program* Technique::permute(Uint64 _flags) const {
  RX_ASSERT(m_type == Type::PERMUTE, "not a permute technique");

  const Size permutations{m_permute_flags.size()};
  for (Size i{0}; i < permutations; i++) {
    if (m_permute_flags[i] != _flags) {
      continue;
    }
    return m_programs[i];
  }

  return nullptr;
}

Program* Technique::variant(Size _index) const {
  RX_ASSERT(m_type == Type::VARIANT, "not a variant technique");
  return m_programs[_index];
}

bool Technique::load(Stream* _stream) {
  m_name = _stream->name();
  auto& allocator = m_frontend->allocator();
  if (auto data = read_text_stream(allocator, _stream)) {
    if (auto disown = data->disown()) {
      return parse({*disown});
    }
  }
  return false;
}

bool Technique::load(const String& _file_name) {
  if (Filesystem::File file{_file_name, "rb"}) {
    return load(&file);
  }
  return false;
}

void Technique::fini() {
  m_programs.each_fwd([this](Program* _program) {
    m_frontend->destroy_program(RX_RENDER_TAG("technique"), _program);
  });

  m_programs.clear();
}

bool Technique::parse(const JSON& _description) {
  if (!_description) {
    const auto json_error{_description.error()};
    if (json_error) {
      return error("%s: %s", m_name, *json_error);
    } else {
      return error("empty description");
    }
  }

  const auto& name{_description["name"]};
  if (!name) {
    return error("missing name");
  }

  if (!name.is_string()) {
    return error("expected String");
  }

  m_name = Utility::move(name.as_string());

  const auto& uniforms{_description["uniforms"]};
  const auto& shaders{_description["shaders"]};
  const auto& permutes{_description["permutes"]};
  const auto& variants{_description["variants"]};

  if (!shaders) {
    return error("missing shaders");
  }

  if (permutes && variants) {
    return error("cannot define both permutes and variants");
  }

  if (uniforms && !parse_uniforms(uniforms)) {
    return false;
  }

  if (!parse_shaders(shaders)) {
    return false;
  }

  if (permutes) {
    if (!parse_specializations(permutes, "permutes")) {
      return false;
    }
    m_type = Type::PERMUTE;
  } else if (variants) {
    if (!parse_specializations(variants, "variants")) {
      return false;
    }
    m_type = Type::VARIANT;
  } else {
    m_type = Type::BASIC;
  }

  return true;
}

void Technique::write_log(Log::Level _level, String&& message_) const {
  if (m_name.is_empty()) {
    logger->write(_level, "%s", Utility::move(message_));
  } else {
    logger->write(_level, "technique '%s': %s", m_name, Utility::move(message_));
  }
}

bool Technique::parse_uniforms(const JSON& _uniforms) {
  if (!_uniforms.is_array_of(JSON::Type::OBJECT)) {
    return error("expected Array[Object] for 'uniforms'");
  }

  return _uniforms.each([this](const JSON& _uniform) {
    return parse_uniform(_uniform);
  });
}

bool Technique::parse_shaders(const JSON& _shaders) {
  if (!_shaders.is_array_of(JSON::Type::OBJECT)) {
    return error("expected Array[Object] for 'shaders'");
  }

  return _shaders.each([this](const JSON& _shader) {
    return parse_shader(_shader);
  });
}

bool Technique::parse_uniform(const JSON& _uniform) {
  if (!_uniform.is_object()) {
    return error("expected Object");
  }

  const auto& name{_uniform["name"]};
  const auto& type{_uniform["type"]};
  const auto& when{_uniform["when"]};
  const auto& value{_uniform["value"]};

  if (!name) {
    return error("missing 'name' in uniform");
  }

  if (!type) {
    return error("missing 'type' in uniform");
  }

  if (!name.is_string()) {
    return error("expected String for 'name'");
  }

  if (!type.is_string()) {
    return error("expected String for 'type'");
  }

  if (when && !when.is_string()) {
    return error("expected String for 'when'");
  }

  const auto name_string{name.as_string()};
  const auto type_string{type.as_string()};

  // Ensure we don't have multiple definitions of the same uniform.
  auto find = [name_string](const UniformDefinition& _uniform_definition) {
    return _uniform_definition.name == name_string;
  };
  if (m_uniform_definitions.find_if(find)) {
    return error("duplicate uniform '%s'", name_string);
  }

  const auto kind = uniform_type_from_string(type_string);
  if (!kind) {
    return error("unknown Type '%s' for '%s'", type_string, name_string);
  }

  UniformDefinition::Variant constant;

  if (value) {
    switch (*kind) {
    case Uniform::Type::SAMPLER1D: [[fallthrough]];
    case Uniform::Type::SAMPLER2D: [[fallthrough]];
    case Uniform::Type::SAMPLER3D: [[fallthrough]];
    case Uniform::Type::SAMPLERCM:
      [[fallthrough]];
    case Uniform::Type::INT:
      if (!value.is_integer()) {
        return error("expected Integer for %s", name_string);
      }
      constant.as_int = value.as_integer();
      break;

    case Uniform::Type::BOOL:
      if (!value.is_boolean()) {
        return error("expected Boolean for %s", name_string);
      }
      constant.as_bool = value.as_boolean();
      break;

    case Uniform::Type::FLOAT:
      if (!value.is_number()) {
        return error("expected Number for %s", name_string);
      }
      constant.as_float = value.as_float();
      break;

    case Uniform::Type::VEC2I:
      if (!value.is_array_of(JSON::Type::INTEGER, 2)) {
        return error("expected Array[Integer, 2] for %s", name_string);
      }
      constant.as_vec2i = {
        value[0_z].as_integer(),
        value[1_z].as_integer()
      };
      break;

    case Uniform::Type::VEC3I:
      if (!value.is_array_of(JSON::Type::INTEGER, 3)) {
        return error("expected Array[Integer, 3] for %s", name_string);
      }
      constant.as_vec3i = {
        value[0_z].as_integer(),
        value[1_z].as_integer(),
        value[2_z].as_integer()
      };
      break;

    case Uniform::Type::VEC4I:
      if (!value.is_array_of(JSON::Type::INTEGER, 4)) {
        return error("expected Array[Integer, 4] for %s", name_string);
      }
      constant.as_vec4i = {
        value[0_z].as_integer(),
        value[1_z].as_integer(),
        value[2_z].as_integer(),
        value[3_z].as_integer()
      };
      break;

    case Uniform::Type::VEC2F:
      if (!value.is_array_of(JSON::Type::NUMBER, 2)) {
        return error("expected Array[Number, 2] for %s", name_string);
      }
      constant.as_vec2f = {
        value[0_z].as_float(),
        value[1_z].as_float()
      };
      break;

    case Uniform::Type::VEC3F:
      if (!value.is_array_of(JSON::Type::NUMBER, 3)) {
        return error("expected Array[Number, 3] for %s", name_string);
      }
      constant.as_vec3f = {
        value[0_z].as_float(),
        value[1_z].as_float(),
        value[2_z].as_float()
      };
      break;

    case Uniform::Type::VEC4F:
      if (!value.is_array_of(JSON::Type::NUMBER, 4)) {
        return error("expected Array[Number, 4] for %s", name_string);
      }
      constant.as_vec4f = {
        value[0_z].as_float(),
        value[1_z].as_float(),
        value[2_z].as_float(),
        value[3_z].as_float()
      };
      break;

    case Uniform::Type::MAT4X4F:
      if (!value.is_array_of(JSON::Type::ARRAY, 4) ||
          !value.each([](const JSON& _row) { return _row.is_array_of(JSON::Type::NUMBER, 4); }))
      {
        return error("expected Array[Array[Number, 4], 4] for %s", name_string);
      }

      constant.as_mat4x4f = {
        {
          value[0_z][0_z].as_float(),
          value[0_z][1_z].as_float(),
          value[0_z][2_z].as_float(),
          value[0_z][3_z].as_float()
        },
        {
          value[1_z][0_z].as_float(),
          value[1_z][1_z].as_float(),
          value[1_z][2_z].as_float(),
          value[1_z][3_z].as_float()
        },
        {
          value[2_z][0_z].as_float(),
          value[2_z][1_z].as_float(),
          value[2_z][2_z].as_float(),
          value[2_z][3_z].as_float()
        },
        {
          value[3_z][0_z].as_float(),
          value[3_z][1_z].as_float(),
          value[3_z][2_z].as_float(),
          value[3_z][3_z].as_float()
        }
      };
      break;

    case Uniform::Type::MAT3X3F:
      if (!value.is_array_of(JSON::Type::ARRAY, 3) ||
          !value.each([](const JSON& _row) { return _row.is_array_of(JSON::Type::NUMBER, 3); }))
      {
        return error("expected Array[Array[Number, 3], 3] for %s", name_string);
      }

    constant.as_mat3x3f = {
        {
          value[0_z][0_z].as_float(),
          value[0_z][1_z].as_float(),
          value[0_z][2_z].as_float()
        },
        {
          value[1_z][0_z].as_float(),
          value[1_z][1_z].as_float(),
          value[1_z][2_z].as_float()
        },
        {
          value[2_z][0_z].as_float(),
          value[2_z][1_z].as_float(),
          value[2_z][2_z].as_float()
        }
      };
      break;

    case Uniform::Type::BONES:
      return error("cannot give value for bones");
    }
  }

  return m_uniform_definitions.emplace_back(*kind, name_string, when ? when.as_string() : "", constant, value ? true : false);
}

bool Technique::parse_shader(const JSON& _shader) {
  if (!_shader.is_object()) {
    return error("expected Object");
  }

  const auto& type{_shader["type"]};
  const auto& source{_shader["source"]};
  const auto& when{_shader["when"]};
  const auto& imports{_shader["imports"]};

  if (!type) {
    return error("missing 'type' in shader");
  }

  if (!source) {
    return error("missing 'source' in shader");
  }

  if (!type.is_string()) {
    return error("expected String for 'type'");
  }

  if (!source.is_string()) {
    return error("expected String for 'source'");
  }

  if (when && !when.is_string()) {
    return error("expected String for 'when'");
  }

  if (imports && !imports.is_array_of(JSON::Type::STRING)) {
    return error("expected Array[String] for 'imports'");
  }

  const auto type_string{type.as_string()};
  Shader::Type shader_type;
  if (type_string == "vertex") {
    shader_type = Shader::Type::VERTEX;
  } else if (type_string == "fragment") {
    shader_type = Shader::Type::FRAGMENT;
  } else {
    return error("unknown Type '%s' for shader", type_string);
  }

  // ensure we don't have multiple definitions of the same shader
  if (!m_shader_definitions.each_fwd([shader_type](const ShaderDefinition& _shader_definition)
    { return _shader_definition.kind != shader_type; }))
  {
    return error("multiple %s shaders present", type_string);
  }

  ShaderDefinition definition;
  definition.kind = shader_type;
  definition.when = when ? when.as_string() : "";
  definition.source = source.as_string();

  if (imports) {
    const auto result{imports.each([this, &definition](const JSON& _import) {
      if (!_import.is_string()) {
        return error("expected String for import");
      }
      return definition.dependencies.push_back(_import.as_string());
    })};

    if (!result) {
      return false;
    }
  }

  const auto& inputs{_shader["inputs"]};
  if (inputs && !parse_inouts(inputs, "input", definition.inputs)) {
    return false;
  }

  const auto& outputs{_shader["outputs"]};
  if (outputs && !parse_inouts(outputs, "output", definition.outputs)) {
    return false;
  }

  return m_shader_definitions.push_back(Utility::move(definition));
}

bool Technique::parse_inouts(const JSON& _inouts, const char* _type,
                             Map<String, ShaderDefinition::InOut>& inouts_)
{
  if (!_inouts.is_array_of(JSON::Type::OBJECT)) {
    return error("expected Array[Object] in %ss", _type);
  }

  Size index = 0;
  return _inouts.each([&](const JSON& _inout) {
    return parse_inout(_inout, _type, inouts_, index);
  });
}

bool Technique::parse_inout(const JSON& _inout, const char* _type,
                            Map<String, ShaderDefinition::InOut>& inouts_,
                            Size& index_)
{
  const auto& name{_inout["name"]};
  const auto& type{_inout["type"]};
  const auto& when{_inout["when"]};

  if (!name) {
    return error("missing 'name' in %s", _type);
  }

  if (!type) {
    return error("missing 'type' in %s", _type);
  }

  if (!name.is_string()) {
    return error("expected String for 'name'");
  }

  if (!type.is_string()) {
    return error("expected String for 'type'");
  }

  if (when && !when.is_string()) {
    return error("expected String for 'when'");
  }

  const auto name_string{name.as_string()};
  if (inouts_.find(name_string)) {
    return error("duplicate '%s'", name_string);
  }

  const auto type_string{type.as_string()};
  const auto kind{inout_type_from_string(type_string)};
  if (!kind) {
    return error("unknown type '%s' for '%s'", type_string, name_string);
  }

  ShaderDefinition::InOut inout;
  inout.index = index_;
  inout.kind = *kind;
  inout.when = when ? when.as_string() : "";

  switch (*kind) {
  case Shader::InOutType::MAT3X3F:
    index_ += 3; // One for each vector.
    break;
  case Shader::InOutType::MAT4X4F:
    index_ += 4; // One for each vector.
    break;
  default:
    index_ += 1;
    break;
  }

  return inouts_.insert(name_string, inout) != nullptr;
}

bool Technique::parse_specializations(const JSON& _specializations,
                                      const char* _type)
{
  if (!_specializations.is_array_of(JSON::Type::STRING)) {
    return error("expected Array[String] for '%ss'", _type);
  }

  return _specializations.each([this, _type](const JSON& _specialization) {
    return parse_specialization(_specialization, _type);
  });
}

bool Technique::parse_specialization(const JSON& _specialization,
                                     const char* _type)
{
  if (!_specialization.is_string()) {
    return error("expected String for '%s'", _type);
  }

  return m_specializations.push_back(_specialization.as_string());
}

bool Technique::resolve_dependencies(const Map<String, Module>& _modules) {
  auto& allocator = m_frontend->allocator();

  // For every shader in technique.
  return m_shader_definitions.each_fwd([&](ShaderDefinition& _shader) {
    Algorithm::TopologicalSort<String> sorter{allocator};
    Set<String> visited{allocator};

    // For every dependency in the shader.
    const bool result = _shader.dependencies.each_fwd([&](const String& _dependency) {
      if (auto find = _modules.find(_dependency)) {
        return resolve_module_dependencies(_modules, *find, visited, sorter);
      }
      return false;
    });

    if (!result) {
      return error("could not satisfy all dependencies");
    }

    auto dependencies = sorter.sort();
    if (!dependencies) {
      return error("out of memory");
    }

    // When cycles are formed in the resolution we cannot satisfy.
    if (dependencies->cycled.size()) {
      auto has_cycles = !dependencies->cycled.each_fwd([&](const String& _module) {
        return error("dependency '%s' forms a cycle", _module);
      });
      if (has_cycles) {
        return false;
      }
    }

    // Fill out the source with all the modules in sorted order.
    String source{allocator};
    if (dependencies->sorted.size()) {
      const char* shader_type{""};
      switch (_shader.kind) {
      case Shader::Type::FRAGMENT:
        shader_type = "fragment";
        break;
      case Shader::Type::VERTEX:
        shader_type = "vertex";
        break;
      }

      logger->verbose("'%s': %s shader has %zu dependencies",
        m_name, shader_type, dependencies->sorted.size());

      auto append_module = dependencies->sorted.each_fwd([&](const String& _module) {
        const auto find = _modules.find(_module);
        if (!find) {
          return error("module '%s' not found", _module);
        }

        logger->verbose("'%s': %s shader requires module '%s'",
          m_name, shader_type, _module);

        return source.append(String::format("// Module %s\n", _module))
            && source.append("// {\n")
            && source.append(find->source())
            && source.append("// }\n");
      });

      if (!append_module) {
        return false;
      }

      if (!source.append(_shader.source)) {
        return false;
      }

      // Replace the shader source with the new injected modules
      _shader.source = Utility::move(source);
    }

    return true;
  });
}

} // namespace Rx::Render::Frontend
