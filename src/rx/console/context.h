#ifndef RX_CONSOLE_CONTEXT_H
#define RX_CONSOLE_CONTEXT_H
#include "rx/console/variable.h"
#include "rx/console/command.h"

#include "rx/core/map.h"

namespace Rx::Console {

struct Token;

struct Context {
  RX_MARK_NO_COPY(Context);
  RX_MARK_NO_MOVE(Context);

  constexpr Context(Memory::Allocator& _allocator);

  [[nodiscard]] bool load(const char* _file_name);
  [[nodiscard]] bool save(const char* _file_name);

  // TODO(dweiler): Figure out how to do multiple Console Context for variables...
  static VariableReference* add_variable(VariableReference* _reference);

  [[nodiscard]]
  Command* add_command(const String& _name, const char* _signature,
    Function<bool(Context& console_, const Vector<Command::Argument>&)>&& function_);

  VariableReference* find_variable_by_name(const String& _name);
  VariableReference* find_variable_by_name(const char* _name);

  bool execute(const String& _contents);

  template<typename... Ts>
  RX_HINT_FORMAT(2, 0)
  bool print(const char* _format, Ts&&... _arguments);

  bool write(const String& _message);

  void clear();
  const Vector<String>& lines();

  Optional<Vector<String>> auto_complete_variables(const String& _prefix);
  Optional<Vector<String>> auto_complete_commands(const String& _prefix);

private:
  // set variable |_reference| with token |_token|
  static VariableStatus set_from_reference_and_token(VariableReference* _reference, const Token& _token);

  // set variable |_reference| with value |_value|
  template<typename T>
  static VariableStatus set_from_reference_and_value(VariableReference* _reference, const T& _value);

  // merge-sort variable references in alphabetical order
  static VariableReference* split(VariableReference* _reference);
  static VariableReference* merge(VariableReference* _lhs, VariableReference* _rhs);
  static VariableReference* sort(VariableReference* _reference);

  // TODO(dweiler): limited line count queue for messages on the console.
  Memory::Allocator& m_allocator;
  Vector<String> m_lines;
  Map<String, Command> m_commands;
};

inline constexpr Context::Context(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_lines{_allocator}
  , m_commands{_allocator}
{
}

template<typename... Ts>
bool Context::print(const char* _format, Ts&&... _arguments) {
  if constexpr(sizeof...(Ts) != 0) {
    return write(String::format(m_allocator, _format, Utility::forward<Ts>(_arguments)...));
  } else {
    return write(_format);
  }
}

inline VariableReference* Context::find_variable_by_name(const String& _name) {
  return find_variable_by_name(_name.data());
}

} // namespace Rx::Console

#endif // RX_CONSOLE_INTERFACE_H
