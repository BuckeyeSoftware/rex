#ifndef RX_CONSOLE_INTERFACE_H
#define RX_CONSOLE_INTERFACE_H
#include "rx/console/variable.h"
#include "rx/console/command.h"

namespace rx::console {

struct token;

struct interface {
  static bool load(const char* _file_name);
  static bool save(const char* _file_name);

  static variable_reference* add_variable(variable_reference* _reference);
  static void add_command(const string& _name, const char* _signature,
    function<bool(const vector<command::argument>&)>&& _function);

  static variable_reference* find_variable_by_name(const string& _name);
  static variable_reference* find_variable_by_name(const char* _name);

  static bool execute(const string& _contents);

  template<typename... Ts>
  static void print(const char* _format, Ts&&... _arguments);
  static void write(const string& _message);
  static void clear();
  static const vector<string>& lines();

  static vector<string> auto_complete_variables(const string& _prefix);
  static vector<string> auto_complete_commands(const string& _prefix);

private:
  // set variable |_reference| with token |_token|
  static variable_status set_from_reference_and_token(variable_reference* _reference, const token& _token);

  // set variable |_reference| with value |_value|
  template<typename T>
  static variable_status set_from_reference_and_value(variable_reference* _reference, const T& _value);

  // merge-sort variable references in alphabetical order
  static variable_reference* split(variable_reference* _reference);
  static variable_reference* merge(variable_reference* _lhs, variable_reference* _rhs);
  static variable_reference* sort(variable_reference* _reference);
};

template<typename... Ts>
inline void interface::print(const char* _format, Ts&&... _arguments) {
  write(string::format(_format, utility::forward<Ts>(_arguments)...));
}

inline variable_reference* interface::find_variable_by_name(const string& _name) {
  return find_variable_by_name(_name.data());
}

} // namespace rx::console

#endif // RX_CONSOLE_INTERFACE_H
