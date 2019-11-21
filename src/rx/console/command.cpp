#include <stdio.h>

#include "rx/console/command.h"

namespace rx::console {

command::argument::~argument() {
  if (type == variable_type::k_string) {
    utility::destruct<string>(&as_string);
  }
}

command::argument::argument(argument&& _argument)
  : type{_argument.type}
{
  switch (type) {
  case variable_type::k_boolean:
    as_boolean = _argument.as_boolean;
    break;
  case variable_type::k_string:
    utility::construct<string>(&as_string, utility::move(_argument.as_string));
    break;
  case variable_type::k_int:
    as_int = _argument.as_int;
    break;
  case variable_type::k_float:
    as_float = _argument.as_float;
    break;
  case variable_type::k_vec4f:
    as_vec4f = _argument.as_vec4f;
    break;
  case variable_type::k_vec4i:
    as_vec4i = _argument.as_vec4i;
    break;
  case variable_type::k_vec3f:
    as_vec3f = _argument.as_vec3f;
    break;
  case variable_type::k_vec3i:
    as_vec3i = _argument.as_vec3i;
    break;
  case variable_type::k_vec2f:
    as_vec2f = _argument.as_vec2f;
    break;
  case variable_type::k_vec2i:
    as_vec2i = _argument.as_vec2i;
    break;
  }
}

command::command(memory::allocator* _allocator, const char* _signature, delegate&& _function)
  : m_delegate{utility::move(_function)}
  , m_arguments{_allocator}
  , m_signature{_signature}
{
}

command::~command() {

}

bool command::execute() {
  // Check the signature of the arguments
  const argument* arg{m_arguments.data()};
  for (const char* ch{m_signature}; *ch; ch++, arg++) {
    switch (*ch) {
    case 'b':
      if (arg->type != variable_type::k_boolean) {
        return false;
      }
      break;
    case 's':
      if (arg->type != variable_type::k_string) {
        return false;
      }
      break;
    case 'i':
      if (arg->type != variable_type::k_int) {
        return false;
      }
      break;
    case 'f':
      if (arg->type != variable_type::k_float) {
        return false;
      }
      break;
    case 'v':
      ch++; // Skip 'v'
      switch (*ch) {
      case '2':
        [[fallthrough]];
      case '3':
        [[fallthrough]];
      case '4':
        ch++; // Skip '2', '3' or '4'
        if (*ch != 'f' && *ch != 'i') {
          return false;
        }
        switch (ch[-1]) {
        case '2':
          if ((*ch == 'f' && arg->type != variable_type::k_vec2f)
           || (*ch == 'i' && arg->type != variable_type::k_vec2i))
          {
            return false;
          }
          break;
        case '3':
          if ((*ch == 'f' && arg->type != variable_type::k_vec3f)
           || (*ch == 'i' && arg->type != variable_type::k_vec3i))
          {
            return false;
          }
          break;
        case '4':
          if ((*ch == 'f' && arg->type != variable_type::k_vec4f)
           || (*ch == 'i' && arg->type != variable_type::k_vec4i))
          {
            return false;
          }
          break;
        }
        break;
      default:
        return false;
      }
      break;
    default:
      return false;
    }
  }

  return m_delegate(m_arguments);
}

bool command::execute_string(const string& _contents) {
  m_arguments.clear();

  if (_contents.size() && !parse_string(_contents)) {
    return false;
  }

  return execute();
}

bool command::parse_string(const string& _contents) {
  return _contents.split(' ').each_fwd([&](const string& _part) {
    int n;
    #define match(_fmt, _count, ...) \
      (_part.scan(_fmt "%n", __VA_ARGS__, &n) == (_count) \
        && n == static_cast<int>(_part.size()))

    if (_part == "true") {
      m_arguments.emplace_back(true);
    } else if (_part == "false") {
      m_arguments.emplace_back(false);
    } else if (_part.begins_with("{") && _part.ends_with("}")) {
      argument arg;
      if (_part.contains(".")) {
        if (match("{%f, %f}", 2, &arg.as_vec2f.x, &arg.as_vec2f.y)) {
          arg.type = variable_type::k_vec2f;
        } else if (match("{%f, %f, %f}", 3, &arg.as_vec3f.x, &arg.as_vec3f.y, &arg.as_vec3f.z)) {
          arg.type = variable_type::k_vec3f;
        } else if (match("{%f, %f, %f, %f}", 4, &arg.as_vec4f.x, &arg.as_vec4f.y, &arg.as_vec4f.z, &arg.as_vec4f.w)) {
          arg.type = variable_type::k_vec4f;
        } else {
          return false;
        }
      } else {
        if (match("{%d, %d}", 2, &arg.as_vec2i.x, &arg.as_vec2i.y)) {
          arg.type = variable_type::k_vec2i;
        } else if (match("{%d, %d, %d}", 3, &arg.as_vec3i.x, &arg.as_vec3i.y, &arg.as_vec3i.z)) {
          arg.type = variable_type::k_vec3i;
        } else if (match("{%d, %d, %d, %d}", 4, &arg.as_vec4i.x, &arg.as_vec4i.y, &arg.as_vec4i.z, &arg.as_vec4i.w)) {
          arg.type = variable_type::k_vec4i;
        } else {
          return false;
        }
      }
      m_arguments.push_back(utility::move(arg));
    } else {
      argument arg;
      if (match("%d", 1, &arg.as_int)) {
        arg.type = variable_type::k_int;
      } else if (match("%f", 1, &arg.as_float)) {
        arg.type = variable_type::k_float;
      } else {
        utility::construct<string>(&arg.as_string, _part);
        arg.type = variable_type::k_string;
      }
      m_arguments.push_back(utility::move(arg));
    }
    return true;
  });
}

} // namespace rx::console
