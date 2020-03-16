#include <string.h> // strchr

#include "rx/console/command.h"
#include "rx/core/log.h"
#include "rx/core/hints/unreachable.h"

namespace rx::console {

RX_LOG("console", logger);

static const char* variable_type_string(variable_type _type) {
  switch (_type) {
  case variable_type::k_boolean:
    return "boolean";
  case variable_type::k_string:
    return "string";
  case variable_type::k_int:
    return "int";
  case variable_type::k_float:
    return "float";
  case variable_type::k_vec4f:
    return "vec4f";
  case variable_type::k_vec4i:
    return "vec4i";
  case variable_type::k_vec3f:
    return "vec3f";
  case variable_type::k_vec3i:
    return "vec3i";
  case variable_type::k_vec2f:
    return "vec2f";
  case variable_type::k_vec2i:
    return "vec2i";
  }
  RX_HINT_UNREACHABLE();
};

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

command::command(memory::allocator* _allocator, const string& _name,
  const char* _signature, delegate&& _function)
  : m_allocator{_allocator}
  , m_delegate{utility::move(_function)}
  , m_arguments{_allocator}
  , m_declaration{_allocator}
  , m_name{_allocator, _name}
  , m_signature{_signature}
  , m_argument_count{0}
{
  m_declaration += m_name;
  m_declaration += '(';

  for (const char* ch{m_signature}; *ch; ch++) {
    switch (*ch) {
    case 'b':
      m_declaration += "bool";
      m_argument_count++;
      break;
    case 'i':
      m_declaration += "int";
      m_argument_count++;
      break;
    case 'f':
      m_declaration += "float";
      m_argument_count++;
      break;
    case 's':
      m_declaration += "string";
      m_argument_count++;
      break;
    case 'v':
      {
        const int dims_ch{*++ch}; // skip 'v'
        const int type_ch{*++ch}; // skip '2, 3, 4'
        if (!strchr("234", dims_ch) || !strchr("if", type_ch)) {
          goto invalid_signature;
        }
        m_declaration += string::format("vec%c%c", dims_ch, type_ch);
        m_argument_count++;
      }
      break;
    default:
      goto invalid_signature;
    }

    if (ch[1] != '\0') {
      m_declaration += ", ";
    }
  }

  m_declaration += ')';

  return;

invalid_signature:
  RX_ASSERT(0, "invalid signature");
}

bool command::execute() {
  // Check the signature of the arguments
  if (m_arguments.size() != m_argument_count) {
    logger->error(
      "arity violation in call, expected %zu parameters, got %zu",
      m_argument_count,
      m_arguments.size());
    return false;
  }

  const argument* arg{m_arguments.data()};
  const char* expected{""};
  for (const char* ch{m_signature}; *ch; ch++, arg++) {
    switch (*ch) {
    case 'b':
      if (arg->type != variable_type::k_boolean) {
        expected = "bool";
        goto error;
      }
      break;
    case 's':
      if (arg->type != variable_type::k_string) {
        expected = "string";
        goto error;
      }
      break;
    case 'i':
      if (arg->type != variable_type::k_int) {
        expected = "int";
        goto error;
      }
      break;
    case 'f':
      if (arg->type != variable_type::k_float) {
        expected = "float";
        goto error;
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
        switch (ch[-1]) {
        case '2':
          if (*ch == 'f' && arg->type != variable_type::k_vec2f) {
            expected = "vec2f";
            goto error;
          } else if (*ch == 'i' && arg->type != variable_type::k_vec2i) {
            expected = "vec2i";
            goto error;
          }
          break;
        case '3':
          if (*ch == 'f' && arg->type != variable_type::k_vec3f) {
            expected = "vec3f";
            goto error;
          } else if (*ch == 'i' && arg->type != variable_type::k_vec3i) {
            expected = "vec3i";
            goto error;
          }
          break;
        case '4':
          if (*ch == 'f' && arg->type != variable_type::k_vec4f) {
            expected = "vec4f";
            goto error;
          } else if (*ch == 'i' && arg->type != variable_type::k_vec4i) {
            expected = "vec4i";
            goto error;
          }
          break;
        }
        break;
      }
      break;
    }
  }

  return m_delegate(m_arguments);

error:
  logger->error(
    "%s: expected '%s' for argument %zu, got '%s' instead",
    m_declaration,
    expected,
    static_cast<rx_size>((arg - &m_arguments.first()) + 1),
    variable_type_string(arg->type));
  return false;
}

bool command::execute_tokens(const vector<token>& _tokens) {
  m_arguments.clear();
  _tokens.each_fwd([&](const token& _token) {
    switch (_token.kind()) {
    case token::type::k_atom:
      m_arguments.emplace_back(_token.as_atom());
      break;
    case token::type::k_string:
      m_arguments.emplace_back(_token.as_string());
      break;
    case token::type::k_boolean:
      m_arguments.emplace_back(_token.as_boolean());
      break;
    case token::type::k_int:
      m_arguments.emplace_back(_token.as_int());
      break;
    case token::type::k_float:
      m_arguments.emplace_back(_token.as_float());
      break;
    case token::type::k_vec4f:
      m_arguments.emplace_back(_token.as_vec4f());
      break;
    case token::type::k_vec4i:
      m_arguments.emplace_back(_token.as_vec4i());
      break;
    case token::type::k_vec3f:
      m_arguments.emplace_back(_token.as_vec3f());
      break;
    case token::type::k_vec3i:
      m_arguments.emplace_back(_token.as_vec3i());
      break;
    case token::type::k_vec2f:
      m_arguments.emplace_back(_token.as_vec2f());
      break;
    case token::type::k_vec2i:
      m_arguments.emplace_back(_token.as_vec2i());
      break;
    }
  });
  return execute();
}

} // namespace rx::console
