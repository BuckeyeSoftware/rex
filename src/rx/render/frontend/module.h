#ifndef RX_RENDER_FRONTEND_MODULE_H
#define RX_RENDER_FRONTEND_MODULE_H
#include "rx/core/string.h"
#include "rx/core/vector.h"
#include "rx/core/report.h"

#include "rx/core/algorithm/topological_sort.h"

namespace Rx { struct JSON; }
namespace Rx::Stream { struct Context; }

namespace Rx::Render::Frontend {

struct Module {
  RX_MARK_NO_COPY(Module);

  Module(Memory::Allocator& _allocator);
  Module(Module&& module_);

  Module& operator=(Module&& module_);

  [[nodiscard]] bool load(Stream::UntrackedStream& _stream);
  [[nodiscard]] bool load(const StringView& _file_name);

  [[nodiscard]] bool parse(const JSON& _description);

  const String& source() const &;
  const String& name() const &;
  const Vector<String>& dependencies() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  Memory::Allocator* m_allocator;
  String m_name;
  String m_source;
  Vector<String> m_dependencies;
  Report m_report;
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

bool resolve_module_dependencies(
  const Map<String, Module>& _modules,
  const Module& _current_module,
  Set<String>& visited_,
  Algorithm::TopologicalSort<String>& sorter_);

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_MODULE_H
