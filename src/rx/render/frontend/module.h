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

  bool load(Stream* _stream);
  bool load(const String& _file_name);

  bool parse(const JSON& _description);

  const String& source() const &;
  const String& name() const &;
  const Vector<String>& dependencies() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) const;

  template<typename... Ts>
  void log(Log::Level _level, const char* _format, Ts&&... _arguments) const;

  void write_log(Log::Level _level, String&& message_) const;

  Ref<Memory::Allocator> m_allocator;
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
  return m_allocator;
}

template<typename... Ts>
inline bool Module::error(const char* _format, Ts&&... _arguments) const {
  log(Log::Level::k_error, "%s",
    String::format(allocator(), _format, Utility::forward<Ts>(_arguments)...));
  return false;
}

template<typename... Ts>
inline void Module::log(Log::Level _level, const char* _format,
                        Ts&&... _arguments) const
{
  write_log(_level, String::format(_format, Utility::forward<Ts>(_arguments)...));
}

bool resolve_module_dependencies(
  const Map<String, Module>& _modules,
  const Module& _current_module,
  Set<String>& visited_,
  Algorithm::TopologicalSort<String>& sorter_);

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_MODULE_H
