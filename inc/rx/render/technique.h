#ifndef RX_RENDER_TECHNIQUE_H
#define RX_RENDER_TECHNIQUE_H

#include <rx/core/string.h>
#include <rx/core/array.h>
#include <rx/core/json.h>
#include <rx/core/map.h>
#include <rx/core/log.h>

#include <rx/render/program.h>

namespace rx::render {

struct frontend;
struct technique;

struct technique : concepts::no_copy {
  technique() = default;
  technique(frontend* _frontend);
  ~technique();

  technique(technique&& _technique);
  technique& operator=(technique&& _technique);

  enum class category {
    k_basic,
    k_variant,
    k_permute
  };

  category type() const;
  bool has_variants() const;
  bool has_permutes() const;

  operator program*() const;

  program* permute(rx_u64 _flags) const;
  program* variant(rx_size _index) const;

  bool load(const string& _file_name);

  const string& name() const;

private:
  struct uniform_definition {
    uniform::category type;
    string when;
  };

  struct shader_definition {
    struct inout {
      shader::inout_category type;
      rx_size index;
      string when;
    };

    shader::category type;
    string source;
    map<string, inout> inputs;
    map<string, inout> outputs;
    string when;
  };

  bool evaluate_when_for_permute(const string& _when, rx_u64 _flags) const;
  bool evaluate_when_for_variant(const string& _when, rx_size _index) const;
  bool evaluate_when_for_basic(const string& _when) const;

  bool parse(const json& _description);
  bool compile();

  bool parse_uniforms(const json& _uniforms);
  bool parse_shaders(const json& _shaders);

  bool parse_uniform(const json& _uniform);
  bool parse_shader(const json& _shader);

  bool parse_inouts(const json& _inouts, const char* _type, map<string, shader_definition::inout>& inouts_);
  bool parse_inout(const json& _inout, const char* _type, map<string, shader_definition::inout>& inouts_);

  bool parse_specializations(const json& _specializations, const char* _type);
  bool parse_specialization(const json& _specialization, const char* _type);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& _message) const;

  frontend* m_frontend;
  category m_type;
  array<program*> m_programs;
  array<rx_u32> m_permute_flags;
  string m_name;
  mutable string m_error;

  array<shader_definition> m_shader_definitions;
  map<string, uniform_definition> m_uniforms;
  array<string> m_specializations;
};

inline const string& technique::name() const {
  return m_name;
}

template<typename... Ts>
inline bool technique::error(const char* _format, Ts&&... _arguments) const {
  m_error = string::format(_format, utility::forward<Ts>(_arguments)...);
  log(log::level::k_error, "%s", m_error);
  return false;
}

template<typename... Ts>
inline void technique::log(log::level _level, const char* _format, Ts&&... _arguments) const {
  write_log(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}

} // namespace rx::render

#endif // RX_RENDER_TECHNIQUE_H