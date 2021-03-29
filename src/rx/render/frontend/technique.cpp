#include <string.h> // strncmp

#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/module.h"

#include "rx/core/json.h"
#include "rx/core/optional.h"
#include "rx/core/filesystem/unbuffered_file.h"
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
    // Samplers.
    {"sampler1D", Uniform::Type::SAMPLER1D},
    {"sampler2D", Uniform::Type::SAMPLER2D},
    {"sampler3D", Uniform::Type::SAMPLER3D},
    {"samplerCM", Uniform::Type::SAMPLERCM},

    // Scalars.
    {"s32",       Uniform::Type::S32},
    {"f32",       Uniform::Type::F32},

    // Vectors.
    {"s32x2",     Uniform::Type::S32x2},
    {"s32x3",     Uniform::Type::S32x3},
    {"s32x4",     Uniform::Type::S32x4},
    {"f32x2",     Uniform::Type::F32x2},
    {"f32x3",     Uniform::Type::F32x3},
    {"f32x4",     Uniform::Type::F32x4},

    // Matrices.
    {"f32x4x4",   Uniform::Type::F32x4x4},
    {"f32x3x3",   Uniform::Type::F32x3x3},
    {"f32x3x4",   Uniform::Type::F32x3x4},
    {"lb_bones",  Uniform::Type::LB_BONES},
    {"dq_bones",  Uniform::Type::DQ_BONES}
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
    // Scalars.
    {"f32",     Shader::InOutType::F32},

    // Vectors.
    {"s32x2",   Shader::InOutType::S32x2},
    {"s32x3",   Shader::InOutType::S32x3},
    {"s32x4",   Shader::InOutType::S32x4},
    {"f32x2",   Shader::InOutType::F32x2},
    {"f32x3",   Shader::InOutType::F32x3},
    {"f32x4",   Shader::InOutType::F32x4},

    // Matrices.
    {"f32x3x3", Shader::InOutType::F32x3X3},
    {"f32x4x4", Shader::InOutType::F32x4X4}
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
  , m_name{m_frontend->allocator()}
  , m_configurations{m_frontend->allocator()}
  , m_shader_definitions{m_frontend->allocator()}
  , m_uniform_definitions{m_frontend->allocator()}
{
}

Technique& Technique::operator=(Technique&& technique_) {
  if (&technique_ == this) {
    return *this;
  }

  m_frontend = Utility::exchange(technique_.m_frontend, nullptr);
  m_name = Utility::move(technique_.m_name);
  m_configurations = Utility::move(technique_.m_configurations);
  m_shader_definitions = Utility::move(technique_.m_shader_definitions);
  m_uniform_definitions = Utility::move(technique_.m_uniform_definitions);

  // Update |m_configuration| references to this Technique instance.
  m_configurations.each_fwd([this](Configuration& configuration_) {
    configuration_.m_technique = this;
  });

  return *this;
}

bool Technique::evaluate_when(const Map<String, bool>& _values, const String& _when) const {
  const auto result = binexp_evaluate(_when.data(), _values);
  if (result < 0) {
    return error("when expression evaluation failed: %s for \"%s\"",
      binexp_result_to_string(result), _when);
  }
  return result;
}

Optional<String> Technique::resolve_source(
  const ShaderDefinition& _definition,
  const Map<String, bool>& _values,
  const Map<String, Module>& _modules) const
{
  auto& allocator = m_frontend->allocator();

  Algorithm::TopologicalSort<String> sorter{allocator};
  Set<String> visited{allocator};

  // For each dependency in shader definition.
  const auto result = _definition.dependencies.each_fwd([&](const ShaderDefinition::Dependency& _dependency) {
    const auto evaluate = binexp_evaluate(_dependency.when.data(), _values);
    if (evaluate < 0) {
      return error("when expression evaluation failed: %s for \"%s\"",
        binexp_result_to_string(evaluate), _dependency.when);
    }

    // Do not need this dependency.
    if (evaluate == 0) {
      return true;
    }

    // Recursively resolve dependencies.
    if (auto find = _modules.find(_dependency.name)) {
      return resolve_module_dependencies(_modules, *find, visited, sorter);
    }

    return false;
  });

  if (!result) {
    (void)error("could not satisfy all dependencies");
    return nullopt;
  }

  // Sort the dependencies in topological order.
  auto dependencies = sorter.sort();
  if (!dependencies) {
    (void)error("out of memory");
    return nullopt;
  }

  // When cycles are formed in the resolution we cannot satisfy.
  if (dependencies->cycled.size()) {
    // Write an error for each dependency that forms a cycle.
    dependencies->cycled.each_fwd([&](const String& _module) {
      (void)error("dependency '%s' forms a cycle", _module);
    });
    return nullopt;
  }

  String special{allocator};
  String modules{allocator};
  auto append = [](String& out_, const String& _next) {
    if (!out_.is_empty() && !out_.append(", ")) {
      return false;
    }
    return out_.append(_next);
  };

  auto append_value = [&](const String& _name, bool _value) {
    return _value ? append(special, _name) : true;
  };

  auto append_module = [&](const String& _module) {
    return append(modules, _module);
  };

  if (!_values.each_pair(append_value)) {
    return nullopt; // OOM.
  }

  if (!dependencies->sorted.each_fwd(append_module)) {
    return nullopt;
  };

  logger->verbose("%s: %s shader [%s] requires [%s]",
    m_name, _definition.kind == Shader::Type::FRAGMENT ?
      "fragment" : "vertex", special, modules);

  String source{allocator};
  auto append_modules = dependencies->sorted.each_fwd([&](const String& _module) {
    const auto find = _modules.find(_module);
    if (!find) {
      return error("module '%s' not found", _module);
    }

    return source.append("// Module ")
        && source.append(_module)
        && source.append("\n// {\n")
        && source.append(find->source())
        && source.append("\n// }\n");
  });

  if (!append_modules) {
    return nullopt;
  }

  if (!source.append(_definition.source)) {
    (void)error("out of memory");
    return nullopt;
  }

  return source;
}

bool Technique::compile(const Map<String, Module>& _modules) {
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
          return error("type mismatch for fragment input '%s'", _name);
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
          return error("type mismatch for vertex output '%s'", _name);
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

  // Create a map of all the configuration names with the value false,
  // passing it into |Configuration::compile|. The compile method will search
  // for the configuration name and mark it as true, using this map to drive
  // the declarative predicate system.
  Map<String, bool> values{m_frontend->allocator()};
  auto add_value = [&](const Configuration& _configuration) {
    return values.insert(_configuration.name(), false);
  };

  if (!m_configurations.each_fwd(add_value)) {
    return false;
  }

  // Compile all configurations.
  return m_configurations.each_fwd([&](Configuration& configuration_) {
    return configuration_.compile(_modules, values);
  });
}

// [Technique::Configuration]
Technique::Configuration::Configuration(Technique* _technique, Type _type, String&& name_)
  : m_technique{_technique}
  , m_type{_type}
  , m_name{Utility::move(name_)}
  , m_programs{_technique->m_frontend->allocator()}
  , m_permute_flags{_technique->m_frontend->allocator()}
  , m_specializations{_technique->m_frontend->allocator()}
{
}

Program* Technique::Configuration::basic() const {
  RX_ASSERT(m_type == Type::BASIC, "not a basic technique");
  return m_programs[0];
}

Program* Technique::Configuration::permute(Uint64 _flags) const {
  RX_ASSERT(m_type == Type::PERMUTE, "not a permute technique");

  const Size permutations = m_permute_flags.size();
  for (Size i = 0; i < permutations; i++) {
    if (m_permute_flags[i] != _flags) {
      continue;
    }
    return m_programs[i];
  }

  return nullptr;
}

Program* Technique::Configuration::variant(Size _index) const {
  RX_ASSERT(m_type == Type::VARIANT, "not a variant technique");
  return m_programs[_index];
}

bool Technique::load(Stream& _stream) {
  m_name = _stream.name();
  auto& allocator = m_frontend->allocator();
  if (auto data = _stream.read_text(allocator)) {
    if (auto disown = data->disown()) {
      if (auto json = JSON::parse(allocator, *disown)) {
        return parse(*json);
      }
    }
  }
  return false;
}

bool Technique::Configuration::compile(const Map<String, Module>& _modules,
  const Map<String, bool>& _values)
{
  auto frontend = m_technique->m_frontend;

  const auto& shader_definitions = m_technique->m_shader_definitions;
  const auto& uniform_definitions = m_technique->m_uniform_definitions;

  // Take a copy of the values and possibly append specializations to it.
  auto values = Utility::copy(_values);
  if (!values) {
    return false;
  }

  // Mark the value inside as true for this configuration.
  *values->find(m_name) = true;

  if (m_type == Type::BASIC) {
    // create and add just a single program to m_programs
    auto program = frontend->create_program(RX_RENDER_TAG("technique"));

    shader_definitions.each_fwd([&](const ShaderDefinition& _shader_definition) {
      if (!m_technique->evaluate_when({frontend->allocator()}, _shader_definition.when)) {
        return true;
      }

      Shader specialized_shader{frontend->allocator()};
      specialized_shader.kind = _shader_definition.kind;

      // Emit #defines
      values->each_pair([&](const String& _key, bool _value) {
        return _value ? specialized_shader.source.append(String::format("#define %s\n", _key)) : true;
      });

      auto source = m_technique->resolve_source(_shader_definition, *values, _modules);
      if (!source || !specialized_shader.source.append(*source)) {
        return false;
      }

      // emit inputs
      _shader_definition.inputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout) {
        if (m_technique->evaluate_when(*values, _inout.when)) {
          specialized_shader.inputs.insert(_name, {_inout.index, _inout.kind});
        }
      });

      // emit outputs
      _shader_definition.outputs.each_pair([&](const String& _name, const ShaderDefinition::InOut& _inout){
        if (m_technique->evaluate_when(*values, _inout.when)) {
          specialized_shader.outputs.insert(_name, {_inout.index, _inout.kind});
        }
      });

      return program->add_shader(Utility::move(specialized_shader));
    });

    uniform_definitions.each_fwd([&](const UniformDefinition& _uniform_definition) {
      auto* uniform = program->add_uniform(_uniform_definition.name,
        _uniform_definition.kind, !m_technique->evaluate_when(*values, _uniform_definition.when));
      if (!uniform) {
        // Out of memory.
        return false;
      }
      if (_uniform_definition.has_value) {
        const auto* data = reinterpret_cast<const Byte*>(&_uniform_definition.value);
        uniform->record_raw(data, uniform->size());
      }
      return true;
    });

    frontend->initialize_program(RX_RENDER_TAG("technique"), program);

    if (!m_programs.push_back(program)) {
      return false;
    }
  } else if (m_type == Type::PERMUTE) {
    const auto mask = (1_u64 << m_specializations.size()) - 1;

    auto generate = [&](Uint64 _flags) {
      // Insert or update specialization values for evaluator.
      for (Size i = 0; i < m_specializations.size(); i++) {
        const bool result = _flags & (1_u64 << i);
        if (auto find = values->find(m_specializations[i])) {
          *find = result;
        } else {
          values->insert(m_specializations[i], result);
        }
      }

      m_permute_flags.push_back(_flags);

      auto program = frontend->create_program(RX_RENDER_TAG("technique"));

      shader_definitions.each_fwd([&](const ShaderDefinition& _shader_definition) {
        if (!m_technique->evaluate_when(*values, _shader_definition.when)) {
          return true;
        }

        Shader specialized_shader{frontend->allocator()};
        specialized_shader.kind = _shader_definition.kind;

        // Emit #defines
        values->each_pair([&](const String& _key, bool _value) {
          return _value ? specialized_shader.source.append(String::format("#define %s\n", _key)) : true;
        });

        /*
        const Size specializations{m_specializations.size()};
        for (Size i{0}; i < specializations; i++) {
          const String& specialication{m_specializations[i]};
          if (_flags & (1_u64 << i)) {
            if (!specialized_shader.source.append(String::format("#define %s\n", specialication))) {
              return false;
            }
          }
        }*/

        // Append shader source.
        auto source = m_technique->resolve_source(_shader_definition, *values, _modules);
        if (!source || !specialized_shader.source.append(*source)) {
          return false;
        }

        auto emit_input = [&](const String& _name, const ShaderDefinition::InOut& _inout) -> bool {
          if (m_technique->evaluate_when(*values, _inout.when)) {
            return specialized_shader.inputs.insert(_name, {_inout.index, _inout.kind});
          }
          return true;
        };

        auto emit_output = [&](const String& _name, const ShaderDefinition::InOut& _inout) -> bool {
          if (m_technique->evaluate_when(*values, _inout.when)) {
            return specialized_shader.outputs.insert(_name, {_inout.index, _inout.kind});
          }
          return true;
        };

        // Emit inputs.
        if (!_shader_definition.inputs.each_pair(emit_input)) {
          return false;
        }

        // Emit outputs.
        if (!_shader_definition.outputs.each_pair(emit_output)) {
          return false;
        }

        return program->add_shader(Utility::move(specialized_shader));
      });

      // emit uniforms
      uniform_definitions.each_fwd([&](const UniformDefinition& _uniform_definition) {
        auto* uniform = program->add_uniform(_uniform_definition.name,
          _uniform_definition.kind, !m_technique->evaluate_when(*values, _uniform_definition.when));
        if (!uniform) {
          // Out of memory.
          return false;
        }
        if (_uniform_definition.has_value) {
          const auto* data = reinterpret_cast<const Byte*>(&_uniform_definition.value);
          uniform->record_raw(data, uniform->size());
        }
        return true;
      });

      // initialize and track
      frontend->initialize_program(RX_RENDER_TAG("technique"), program);

      if (!m_programs.push_back(program)) {
        return false;
      }

      return true;
    };

    for (Uint64 flags = 0; flags != mask; flags = ((flags | ~mask) + 1_u64) & mask) {
      generate(flags);
    }

    generate(mask);
  } else if (m_type == Type::VARIANT) {
    const auto specializations = m_specializations.size();

    // Set all the values to false. On each iteration temporarily mark the
    // specialization as true, this avoids needing to construct a map each
    // iteration.
    for (Size j = 0; j < specializations; j++) {
      values->insert(m_specializations[j], false);
    }

    for (Size i = 0; i < specializations; i++) {
      // const auto& specialization = m_specializations[i];

      // This can never fail.
      auto value = values->find(m_specializations[i]);
      RX_ASSERT(value, "should never fail");

      *value = true;

      auto program = frontend->create_program(RX_RENDER_TAG("technique"));

      shader_definitions.each_fwd([&](const ShaderDefinition& _shader_definition) {
        if (!m_technique->evaluate_when(*values, _shader_definition.when)) {
          return true;
        }

        Shader specialized_shader{frontend->allocator()};
        specialized_shader.kind = _shader_definition.kind;

        values->each_pair([&](const String& _key, bool _value) {
          return _value ? specialized_shader.source.append(String::format("#define %s\n", _key)) : true;
        });

        // Emit specialization #define
        // if (!specialized_shader.source.append(String::format("#define %s\n", specialization))) {
        //   return false;
        // }

        // Append shader source.
        auto source = m_technique->resolve_source(_shader_definition, *values, _modules);
        if (!source || !specialized_shader.source.append(*source)) {
          return false;
        }

        auto emit_input = [&](const String& _name, const ShaderDefinition::InOut& _inout) -> bool {
          if (m_technique->evaluate_when(*values, _inout.when)) {
            return specialized_shader.inputs.insert(_name, {_inout.index, _inout.kind});
          }
          return true;
        };

        auto emit_output = [&](const String& _name, const ShaderDefinition::InOut& _inout) -> bool {
          if (m_technique->evaluate_when(*values, _inout.when)) {
            return specialized_shader.outputs.insert(_name, {_inout.index, _inout.kind});
          }
          return true;
        };

        // Emit inputs.
        if (!_shader_definition.inputs.each_pair(emit_input)) {
          return false;
        }

        // Emit outputs.
        if (!_shader_definition.outputs.each_pair(emit_output)) {
          return false;
        }

        return program->add_shader(Utility::move(specialized_shader));
      });

      // Emit uniforms.
      uniform_definitions.each_fwd([&](const UniformDefinition& _uniform_definition) {
        auto* uniform = program->add_uniform(_uniform_definition.name,
          _uniform_definition.kind, !m_technique->evaluate_when(*values, _uniform_definition.when));
        if (!uniform) {
          // Out of memory.
          return false;
        }
        if (_uniform_definition.has_value) {
          const auto* data = reinterpret_cast<const Byte*>(&_uniform_definition.value);
          uniform->record_raw(data, uniform->size());
        }
        return true;
      });

      // initialize and track
      frontend->initialize_program(RX_RENDER_TAG("technique"), program);

      if (!m_programs.push_back(program)) {
        return false;
      }

      *value = false;
    };
  }

  return true;
}

void Technique::Configuration::release() {
  if (!m_technique) {
    return;
  }

  m_programs.each_fwd([this](Program* _program) {
    m_technique->m_frontend->destroy_program(RX_RENDER_TAG("technique"), _program);
  });

  m_programs.clear();
}

bool Technique::Configuration::parse_specializations(
  const JSON& _specializations, const char* _type)
{
  if (!_specializations.is_array_of(JSON::Type::STRING)) {
    return m_technique->error("expected Array[String] for '%ss'", _type);
  }

  return _specializations.each([this, _type](const JSON& _specialization) {
    return parse_specialization(_specialization, _type);
  });
}

bool Technique::Configuration::parse_specialization(
  const JSON& _specialization, const char* _type)
{
  if (!_specialization.is_string()) {
    return m_technique->error("expected String for '%s'", _type);
  }

  return m_specializations.push_back(_specialization.as_string());
}

bool Technique::load(const String& _file_name) {
  auto& allocator = m_frontend->allocator();
  if (auto file = Filesystem::UnbufferedFile::open(allocator, _file_name, "r")) {
    return load(*file);
  }
  return false;
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
  const auto& configurations{_description["configurations"]};

  if (!shaders) {
    return error("missing shaders");
  }

  if (!configurations) {
    return error("missing configurations");
  }

  if (uniforms && !parse_uniforms(uniforms)) {
    return false;
  }

  if (!parse_shaders(shaders)) {
    return false;
  }

  if (!parse_configurations(configurations)) {
    return false;
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

bool Technique::parse_configurations(const JSON& _configurations) {
  if (!_configurations.is_array_of(JSON::Type::OBJECT)) {
    return error("expected Array[Object] for 'configurations'");
  }

  return _configurations.each([this](const JSON& _configuration) {
    return parse_configuration(_configuration);
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
    return error("unknown type '%s' for '%s'", type_string, name_string);
  }

  UniformDefinition::Variant constant;

  if (value) {
    switch (*kind) {
    case Uniform::Type::SAMPLER1D: [[fallthrough]];
    case Uniform::Type::SAMPLER2D: [[fallthrough]];
    case Uniform::Type::SAMPLER3D: [[fallthrough]];
    case Uniform::Type::SAMPLERCM:
      [[fallthrough]];
    case Uniform::Type::S32:
      if (!value.is_integer()) {
        return error("expected Integer for %s", name_string);
      }
      constant.as_int = value.as_integer();
      break;
    case Uniform::Type::F32:
      if (!value.is_number()) {
        return error("expected Number for %s", name_string);
      }
      constant.as_float = value.as_float();
      break;
    case Uniform::Type::S32x2:
      if (!value.is_array_of(JSON::Type::INTEGER, 2)) {
        return error("expected Array[Integer, 2] for %s", name_string);
      }
      constant.as_vec2i = {
        value[0_z].as_integer(),
        value[1_z].as_integer()
      };
      break;
    case Uniform::Type::S32x3:
      if (!value.is_array_of(JSON::Type::INTEGER, 3)) {
        return error("expected Array[Integer, 3] for %s", name_string);
      }
      constant.as_vec3i = {
        value[0_z].as_integer(),
        value[1_z].as_integer(),
        value[2_z].as_integer()
      };
      break;
    case Uniform::Type::S32x4:
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
    case Uniform::Type::F32x2:
      if (!value.is_array_of(JSON::Type::NUMBER, 2)) {
        return error("expected Array[Number, 2] for %s", name_string);
      }
      constant.as_vec2f = {
        value[0_z].as_float(),
        value[1_z].as_float()
      };
      break;
    case Uniform::Type::F32x3:
      if (!value.is_array_of(JSON::Type::NUMBER, 3)) {
        return error("expected Array[Number, 3] for %s", name_string);
      }
      constant.as_vec3f = {
        value[0_z].as_float(),
        value[1_z].as_float(),
        value[2_z].as_float()
      };
      break;

    case Uniform::Type::F32x4:
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
    case Uniform::Type::F32x4x4:
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
    case Uniform::Type::F32x3x3:
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
    case Uniform::Type::F32x3x4:
      return error("cannot specify value for non-square matrices");
    case Uniform::Type::LB_BONES:
      [[fallthrough]];
    case Uniform::Type::DQ_BONES:
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

  if (imports && !imports.is_array()) {
    return error("expected Array[String | Object] for 'imports'");
  }

  const auto type_string{type.as_string()};
  Shader::Type shader_type;
  if (type_string == "vertex") {
    shader_type = Shader::Type::VERTEX;
  } else if (type_string == "fragment") {
    shader_type = Shader::Type::FRAGMENT;
  } else {
    return error("unknown type '%s' for shader", type_string);
  }

  // Ensure we don't have multiple definitions of the same shader.
  if (!m_shader_definitions.each_fwd([shader_type](const ShaderDefinition& _shader_definition)
    { return _shader_definition.kind != shader_type; }))
  {
    return error("multiple %s shaders present", type_string);
  }

  ShaderDefinition definition{m_frontend->allocator()};
  definition.kind = shader_type;
  definition.when = when ? when.as_string() : "";
  definition.source = source.as_string();

  if (imports) {
    const auto result{imports.each([this, &definition](const JSON& _import) {
      if (_import.is_string()) {
        return definition.dependencies.emplace_back(_import.as_string(), "");
      } else if (_import.is_object()) {
        const auto& name = _import["name"];
        const auto& when = _import["when"];
        if (!name) {
          return error("expected 'name' for import");
        }
        if (!when) {
          return error("expected 'when' for import");
        }
        if (!name.is_string()) {
          return error("expected String for 'name' in import");
        }
        if (!when.is_string()) {
          return error("expected String for 'when' in import");
        }
        return definition.dependencies.emplace_back(name.as_string(), when.as_string());
      }
      return error("expected String or Object for import");
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

bool Technique::parse_configuration(const JSON& _configuration) {
  const auto& name = _configuration["name"];
  const auto& permutes = _configuration["permutes"];
  const auto& variants = _configuration["variants"];

  if (!name) {
    return error("expected name for configuration");
  }

  if (!name.is_string()) {
    return error("expected String for 'name' in configuration");
  }

  if (permutes && variants) {
    return error("cannot define both permutes and variants");
  }

  if (permutes) {
    if (!m_configurations.emplace_back(this, Configuration::Type::PERMUTE, name.as_string())) {
      return false;
    }
    if (!m_configurations.last().parse_specializations(permutes, "permutes")) {
      return false;
    }
  } else if (variants) {
    if (!m_configurations.emplace_back(this, Configuration::Type::VARIANT, name.as_string())) {
      return false;
    }
    if (!m_configurations.last().parse_specializations(variants, "variants")) {
      return false;
    }
  } else if (!m_configurations.emplace_back(this, Configuration::Type::BASIC, name.as_string())) {
    return false;
  }

  return true;
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
  case Shader::InOutType::F32x3X3:
    index_ += 3; // One for each vector.
    break;
  case Shader::InOutType::F32x4X4:
    index_ += 4; // One for each vector.
    break;
  default:
    index_ += 1;
    break;
  }

  return inouts_.insert(name_string, inout) != nullptr;
}

} // namespace Rx::Render::Frontend
