#ifndef RX_RENDER_FRONTEND_MODULE_H
#define RX_RENDER_FRONTEND_MODULE_H
#include "rx/core/string.h"
#include "rx/core/vector.h"
#include "rx/core/log.h"

#include "rx/core/algorithm/topological_sort.h"

namespace Rx {
  struct JSON;
  struct Stream;
}

namespace Rx::Render::Frontend {

struct Module {
  RX_MARK_NO_COPY(Module);

  Module(Memory::Allocator& _allocator);
  Module(Module&& module_);

  Module& operator=(Module&& module_);

  [[nodiscard]] bool load(Stream& _stream);
  [[nodiscard]] bool load(const String& _file_name);

  [[nodiscard]] bool parse(const JSON& _description);

  const String& source() const &;
  const String& name() const &;
  const Vector<String>& dependencies() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  template<typename... Ts>
  [[nodiscard]] RX_HINT_FORMAT(2, 0)
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  RX_HINT_FORMAT(3, 0)
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  Memory::Allocator* m_allocator;
  String m_name;
  String m_source;
  Vector<String> m_dependencies;
};

inline const String& Module::source() const & {
  return m_source;
}

inline const String& Module::name() const & {
  return m_name;
}

inline const Vector<String>& Module::dependencies() const & {
  return m_dependencies;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Module::allocator() const {
  return *m_allocator;
}

template<typename... Ts>
bool Module::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::ERROR, _format, Utility::forward<Ts>(_arguments)...);
  return false;
}

template<typename... Ts>
void Module::log(Log::Level _level, const char* _format, Ts&&... _arguments) const {
  if constexpr (sizeof...(Ts) != 0) {
    write_log(_level, String::format(_format, Utility::forward<Ts>(_arguments)...));
  } else {
    write_log(_level, _format);
  }
}

bool resolve_module_dependencies(
  const Map<String, Module>& _modules,
  const Module& _current_module,
  Set<String>& visited_,
  Algorithm::TopologicalSort<String>& sorter_);

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_MODULE_H
