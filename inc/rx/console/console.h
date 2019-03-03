#ifndef RX_CONSOLE_CONSOLE_H
#define RX_CONSOLE_CONSOLE_H

namespace rx::console {

struct variable_reference;

struct console {
  static bool load(const char* file_name);
  static bool save(const char* file_name);

  static variable_reference* add_variable_reference(variable_reference* reference);

private:
  // linked-list merge sort for variable references
  static variable_reference* split(variable_reference* reference);
  static variable_reference* merge(variable_reference* lhs, variable_reference* rhs);
  static variable_reference* sort(variable_reference* reference);
};

} // namespace rx::console

#endif // RX_CONSOLE_CONSOLE_H
