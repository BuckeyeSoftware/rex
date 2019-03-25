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

  program* operator[](rx_u64 _flags) const;
  program* operator[](const char* _variant) const;

  bool load(const string& _file_name);

  const string& name() const;

private:
  bool parse(const json& _description);
  bool compile();

  bool parse_uniforms(const json& _uniforms);
  bool parse_shaders(const json& _shaders);

  bool parse_uniform(const json& _uniform);
  bool parse_shader(const json& _shader);

  bool parse_vertex_shader(const json& _shader);
  bool parse_fragment_shader(const json& _shader);

  bool parse_inouts(const json& _inouts, const char* _type, map<string, shader::inout>& inouts_);
  bool parse_inout(const json& _inout, const char* _type, map<string, shader::inout>& inouts_);

  bool parse_specializations(const json& _specializations, const char* _type);
  bool parse_specialization(const json& _specialization, const char* _type);

  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments);

  void write_log(log::level _level, string&& _message);

  frontend* m_frontend;
  category m_type;
  array<program*> m_programs;
  string m_name;
  string m_error;

  array<shader> m_shader_definitions;
  map<string, uniform::category> m_uniforms;
  array<string> m_specializations;
};

inline const string& technique::name() const {
  return m_name;
}

template<typename... Ts>
inline bool technique::error(const char* _format, Ts&&... _arguments) {
  m_error = string::format(_format, utility::forward<Ts>(_arguments)...);
  log(log::level::k_error, "%s", m_error);
  return false;
}

template<typename... Ts>
inline void technique::log(log::level _level, const char* _format, Ts&&... _arguments) {
  write_log(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}

} // namespace rx::render

#endif // RX_RENDER_TECHNIQUE_H