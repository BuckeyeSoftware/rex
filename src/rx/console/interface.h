#ifndef RX_CONSOLE_INTERFACE_H
#define RX_CONSOLE_INTERFACE_H

#include "rx/console/variable.h"

namespace rx::console {

struct interface {
  // loads |_file_name| configuration
  static bool load(const char* _file_name);

  // saves configuration to |_file_name|
  static bool save(const char* _file_name);

  // thread type-erased static global variable |_reference| during static
  // initialization into global linked-list representation
  static variable_reference* add_variable_reference(variable_reference* _reference);

  // set variable |_name| with typed value |_value|
  template<typename T>
  static variable_status set_from_value(const string& _name, const T& _value);

  // change variable |_name| with string encoded |_value|
  static variable_status change(const string& _name, const string& _value);

  // get variable |_name|
  static variable_reference* get_from_name(const string& _name);

private:
  // parse string |_contents| into typed value |value_|
  template<typename T>
  static variable_status parse_string(const string& _contents, T& value_);

  // set variable |_name| with string encoded |_value|
  template<typename T>
  static variable_status set_from_string(const string& _name, const string& _value);

  // merge-sort variable references in alphabetical order
  static variable_reference* split(variable_reference* _reference);
  static variable_reference* merge(variable_reference* _lhs, variable_reference* _rhs);
  static variable_reference* sort(variable_reference* _reference);
};

} // namespace rx::console

#endif // RX_CONSOLE_INTERFACE_H
