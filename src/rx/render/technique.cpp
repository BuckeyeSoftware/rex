#include <rx/render/technique.h>
#include <rx/render/frontend.h>

#include <rx/core/log.h>
#include <rx/core/optional.h>

RX_LOG("render/technique", log_technique);

namespace rx::render {

optional<uniform::category> uniform_category_from_string(const string& _category) {
  static constexpr const struct {
    const char* match;
    uniform::category category;
  } k_table[]{
    {"sampler1D",  uniform::category::k_sampler1D},
    {"sampler2D",  uniform::category::k_sampler2D},
    {"sampler3D",  uniform::category::k_sampler3D},
    {"samplerCM",  uniform::category::k_samplerCM},
    {"bool",       uniform::category::k_bool},
    {"float",      uniform::category::k_float},
    {"vec2i",      uniform::category::k_vec2i},
    {"vec3i",      uniform::category::k_vec3i},
    {"vec4i",      uniform::category::k_vec4i},
    {"vec2f",      uniform::category::k_vec2f},
    {"vec3f",      uniform::category::k_vec3f},
    {"vec4f",      uniform::category::k_vec4f},
    {"mat4x4f",    uniform::category::k_mat4x4f},
    {"mat3x3f",    uniform::category::k_mat3x3f}
  };

  for (const auto& element : k_table) {
    if (_category == element.match) {
      return element.category;
    }
  }

  return nullopt;
}

optional<shader::inout_category> inout_category_from_string(const string& _category) {
  static constexpr const struct {
    const char* match;
    shader::inout_category category;
  } k_table[]{
    {"vec2i", shader::inout_category::k_vec2i},
    {"vec3i", shader::inout_category::k_vec3i},
    {"vec4i", shader::inout_category::k_vec4i},
    {"vec2f", shader::inout_category::k_vec2f},
    {"vec3f", shader::inout_category::k_vec3f},
    {"vec4f", shader::inout_category::k_vec4f},
  };

  for (const auto& element : k_table) {
    if (element.match == _category) {
      return element.category;
    }
  }

  return nullopt;
}

technique::technique(frontend* _frontend, const json& _description)
  : m_frontend{_frontend}
{
  if (parse(_description)) {
    compile();
  }
}

technique::~technique() {
  m_programs.each_fwd([this](program* _program) {
    m_frontend->destroy_program(RX_RENDER_TAG("technique"), _program);
  });
}

bool technique::compile() {
  shader description;

  shader* vertex{nullptr};
  shader* fragment{nullptr};
  m_shader_definitions.each_fwd([&](shader& _shader) {
    if (_shader.type == shader::category::k_fragment) {
      fragment = &_shader;
    } else if (_shader.type == shader::category::k_vertex) {
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
      vertex->outputs.each([&](rx_size, const string& _name, shader::inout_category _category) {
        const auto check{fragment->inputs.find(_name)};
        if (!check) {
          return error("could not find fragment input for vertex output '%s'", _name);
        }
        if (*check != _category) {
          return error("type mismatch for fragment input '%s'", _name);
        }
        return true;
      })
      &&
      // enumerate all fragment inputs and check for matching vertex outputs
      fragment->inputs.each([&](rx_size, const string& _name, shader::inout_category _category) {
        const auto check{vertex->outputs.find(_name)};
        if (!check) {
          return error("could not find vertex output for fragment input '%s'", _name);
        }
        if (*check != _category) {
          return error("type mismatch for vertex output '%s'", _name);
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

  if (m_type == category::k_basic) {
    // create and add just a single program to m_programs
    auto program{m_frontend->create_program(RX_RENDER_TAG("technique"))};

    m_shader_definitions.each_fwd([program](shader& _shader) {
      program->add_shader(utility::move(_shader));
    });

    m_uniforms.each([program](rx_size, const string& _name, uniform::category _type) {
      program->add_uniform(_name, _type);
    });

    m_frontend->initialize_program(RX_RENDER_TAG("technique"), program);
    m_programs.push_back(program);
  } else if (m_type == category::k_permute) {
    // create and add all possible permutations of m_specializations
    // TODO
  } else if (m_type == category::k_variant) {
    // create and add a program for each in m_specializations
    // TODO
  }

  return true;
}

technique::operator program*() const {
  RX_ASSERT(m_type == category::k_basic, "not a basic technique");
  return m_programs[0];
}

program* technique::operator[](rx_u64 _flags) const {
  RX_ASSERT(m_type == category::k_permute, "not a permute technique");
  (void)_flags;
  // TODO
  return nullptr;
}

program* technique::operator[](const char* _variant) const {
  RX_ASSERT(m_type == category::k_variant, "not a variant technique");
  (void)_variant;
  // TODO
  return nullptr;
}

bool technique::parse(const json& _description) {
  if (!_description) {
    const auto json_error{_description.error()};
    if (json_error) {
      return error("%s", *json_error);
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

  m_name = utility::move(name.as_string());

  log(log::level::k_verbose, "parsing ...");

  const auto& uniforms{_description["uniforms"]};
  const auto& shaders{_description["shaders"]};
  const auto& permutes{_description["permutes"]};
  const auto& variants{_description["variants"]};

  if (!uniforms) {
    return error("missing uniforms");
  }

  if (!shaders) {
    return error("missing shaders");
  }

  if (permutes && variants) {
    return error("cannot define both permutes and variants");
  }

  if (!parse_uniforms(uniforms)) {
    return false;
  }

  if (!parse_shaders(shaders)) {
    return false;
  }

  if (permutes) {
    if (!parse_specializations(permutes, "permute")) {
      return false;
    }
    m_type = category::k_permute;
  } else if (variants) {
    if (!parse_specializations(variants, "variant")) {
      return false;
    }
    m_type = category::k_variant;
  } else {
    m_type = category::k_basic;
  }

  return true;
}

void technique::write_log(log::level _level, string&& _message) {
  if (m_name.is_empty()) {
    log_technique(_level, "%s", _message);
  } else {
    log_technique(_level, "technique '%s': %s", m_name, _message);
  }
}

bool technique::parse_uniforms(const json& _uniforms) {
  log(log::level::k_verbose, "parsing uniforms ...");

  if (!_uniforms.is_array()) {
    return error("expected Array for 'uniforms'");
  }

  return _uniforms.each([this](const json& _uniform) {
    return parse_uniform(_uniform);
  });
}

bool technique::parse_shaders(const json& _shaders) {
  log(log::level::k_verbose, "parsing shaders ...");

  if (!_shaders.is_array()) {
    return error("expected Array for 'shaders'");
  }

  return _shaders.each([this](const json& _shader) {
    return parse_shader(_shader);
  });
}

bool technique::parse_uniform(const json& _uniform) {
  if (!_uniform.is_object()) {
    return error("expected Object");
  }

  const auto& name{_uniform["name"]};
  const auto& type{_uniform["type"]};

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

  auto name_string{name.as_string()};
  auto type_string{type.as_string()};

  if (m_uniforms.find(name_string)) {
    return error("duplicate uniform '%s'", name_string);
  }

  auto type_category{uniform_category_from_string(type_string)};
  if (!type_category) {
    return error("unknown type '%s' for '%s'", type_string, name_string);
  }

  m_uniforms.insert(name_string, *type_category);
  log(log::level::k_verbose, "added uniform '%s %s'", type_string, name_string);

  return true;
}

bool technique::parse_shader(const json& _shader) {
  if (!_shader.is_object()) {
    return error("expected Object");
  }

  const auto& type{_shader["type"]};
  const auto& source{_shader["source"]};

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

  const auto type_string{type.as_string()};
  log(log::level::k_verbose, "parsing %s shader ...", type_string);
  if (type_string == "vertex") {
    return parse_vertex_shader(_shader);
  } else if (type_string == "fragment") {
    return parse_fragment_shader(_shader);
  }

  return error("unknown type '%s' for shader", type_string);
}

bool technique::parse_vertex_shader(const json& _shader) {
  if (!m_shader_definitions.each_fwd([this](const shader& _shader) {
      return _shader.type != shader::category::k_vertex; }))
  {
    return error("multiple vertex shaders present");
  }

  shader definition;
  definition.type = shader::category::k_vertex;
  definition.source = _shader["source"].as_string();

  const auto& inputs{_shader["inputs"]};
  if (inputs && !parse_inouts(inputs, "input", definition.inputs)) {
    return false;
  }

  const auto& outputs{_shader["outputs"]};
  if (outputs && !parse_inouts(outputs, "output", definition.outputs)) {
    return false;
  }

  m_shader_definitions.push_back(utility::move(definition));
  log(log::level::k_verbose, "added vertex shader");

  return true;
}

bool technique::parse_fragment_shader(const json& _shader) {
  if (!m_shader_definitions.each_fwd([this](const shader& _shader) {
      return _shader.type != shader::category::k_fragment; }))
  {
    return error("multiple vertex shaders present");
  }

  const auto& inputs{_shader["inputs"]};
  const auto& outputs{_shader["outputs"]};

  shader definition;
  definition.type = shader::category::k_fragment;
  definition.source = _shader["source"].as_string();

  if (inputs && !parse_inouts(inputs, "input", definition.inputs)) {
    return false;
  }

  if (outputs && !parse_inouts(outputs, "output", definition.outputs)) {
    return false;
  }

  m_shader_definitions.push_back(utility::move(definition));
  log(log::level::k_verbose, "added fragment shader");

  return true;
}

bool technique::parse_inouts(const json& _inouts, const char* _type, map<string, shader::inout_category>& inouts_) {
  log(log::level::k_verbose, "parsing %ss ...", _type);

  if (!_inouts.is_array()) {
    return error("expected Array in %ss", _type);
  }

  return _inouts.each([&](const json& _inout) {
    return parse_inout(_inout, _type, inouts_);
  });
}

bool technique::parse_inout(const json& _inout, const char* _type, map<string, shader::inout_category>& inouts_) {
  if (!_inout.is_object()) {
    return error("expected Object for %s", _type);
  }

  const auto& name{_inout["name"]};
  const auto& type{_inout["type"]};

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

  const auto name_string{name.as_string()};
  if (inouts_.find(name_string)) {
    return error("duplicate '%s'", name_string);
  }

  const auto type_string{type.as_string()};
  const auto type_category{inout_category_from_string(type_string)};
  if (!type_category) {
    return error("unknown type '%s' for '%s'", type_string, name_string);
  }

  inouts_.insert(name_string, *type_category);
  log(log::level::k_verbose, "added %s '%s'", _type, name_string);

  return true;
}

bool technique::parse_specializations(const json& _specializations, const char* _type) {
  if (!_specializations.is_array()) {
    return error("expected Array for '%ss'", _type);
  }

  return _specializations.each([this, _type](const json& _specialization) {
    return parse_specialization(_specialization, _type);
  });
}

bool technique::parse_specialization(const json& _specialization, const char* _type) {
  if (!_specialization.is_string()) {
    return error("expected String for '%s'", _type);
  }

  m_specializations.push_back(_specialization.as_string());
  return true;
}

} // namespace rx::render