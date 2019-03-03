#ifndef RX_CONSOLE_CONSOLE_H
#define RX_CONSOLE_CONSOLE_H

#include <rx/console/variable.h>

namespace rx::console {

struct console {
  static bool load(const char* file_name);
  static bool save(const char* file_name);

  static variable_reference* add_variable_reference(variable_reference* reference);

  template<typename T>
  static variable_status set_from_value(const string& name, const T& value);


  static variable_status change(const string& name, const string& value);

private:
  template<typename T>
  static variable_status parse_string(const string& contents, T& value);

  template<typename T>
  static variable_status set_from_string(const string& name, const string& value);

  // linked-list merge sort for variable references
  static variable_reference* split(variable_reference* reference);
  static variable_reference* merge(variable_reference* lhs, variable_reference* rhs);
  static variable_reference* sort(variable_reference* reference);
};

} // namespace rx::console

#endif // RX_CONSOLE_CONSOLE_H
