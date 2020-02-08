#ifndef RX_RENDER_FRONTEND_MODULE_H
#define RX_RENDER_FRONTEND_MODULE_H
#include "rx/core/string.h"
#include "rx/core/vector.h"
#include "rx/core/log.h"

#include "rx/core/algorithm/topological_sort.h"

namespace rx {
  struct json;
  struct stream;
}

namespace rx::render::frontend {

struct module {
  module(memory::allocator* _allocator);

  bool load(stream* _stream);
  bool load(const string& _file_name);

  bool parse(const json& _description);

  const string& source() const &;
  const string& name() const &;
  const vector<string>& dependencies() const &;

  memory::allocator* allocator() const;

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(log::level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(log::level _level, string&& message_) const;

  memory::allocator* m_allocator;
  string m_name;
  string m_source;
  vector<string> m_dependencies;
};

bool resolve_module_dependencies(
  const map<string, module>& _modules,
  const module& _current_module,
  set<string>& visited_,
  algorithm::topological_sort<string>& sorter_);

inline module::module(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_name{m_allocator}
  , m_source{m_allocator}
  , m_dependencies{m_allocator}
{
}

inline const string& module::source() const & {
  return m_source;
}

inline const string& module::name() const & {
  return m_name;
}

inline const vector<string>& module::dependencies() const & {
  return m_dependencies;
}

inline memory::allocator* module::allocator() const {
  return m_allocator;
}

template<typename... Ts>
inline bool module::error(const char* _format, Ts&&... _arguments) const {
  log(log::level::k_error, "%s", string::format(_format, utility::forward<Ts>(_arguments)...));
  return false;
}

template<typename... Ts>
inline void module::log(log::level _level, const char* _format,
  Ts&&... _arguments) const
{
  write_log(_level, string::format(_format, utility::forward<Ts>(_arguments)...));
}


} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_MODULE_H
