#include <string.h> // strchr

#include "rx/console/command.h"
#include "rx/core/log.h"
#include "rx/core/hints/unreachable.h"

namespace Rx::Console {

RX_LOG("console", logger);

static const char* variable_type_string(VariableType _type) {
  switch (_type) {
  case VariableType::BOOLEAN:
    return "boolean";
  case VariableType::STRING:
    return "string";
  case VariableType::INT:
    return "int";
  case VariableType::FLOAT:
    return "float";
  case VariableType::VEC4F:
    return "vec4f";
  case VariableType::VEC4I:
    return "vec4i";
  case VariableType::VEC3F:
    return "vec3f";
  case VariableType::VEC3I:
    return "vec3i";
  case VariableType::VEC2F:
    return "vec2f";
  case VariableType::VEC2I:
    return "vec2i";
  }
  RX_HINT_UNREACHABLE();
};

Command::Argument::~Argument() {
  if (type == VariableType::STRING) {
    Utility::destruct<String>(&as_string);
  }
}

Command::Argument::Argument(Argument&& _argument)
  : type{_argument.type}
{
  switch (type) {
  case VariableType::BOOLEAN:
    as_boolean = _argument.as_boolean;
    break;
  case VariableType::STRING:
    Utility::construct<String>(&as_string, Utility::move(_argument.as_string));
    break;
  case VariableType::INT:
    as_int = _argument.as_int;
    break;
  case VariableType::FLOAT:
    as_float = _argument.as_float;
    break;
  case VariableType::VEC4F:
    as_vec4f = _argument.as_vec4f;
    break;
  case VariableType::VEC4I:
    as_vec4i = _argument.as_vec4i;
    break;
  case VariableType::VEC3F:
    as_vec3f = _argument.as_vec3f;
    break;
  case VariableType::VEC3I:
    as_vec3i = _argument.as_vec3i;
    break;
  case VariableType::VEC2F:
    as_vec2f = _argument.as_vec2f;
    break;
  case VariableType::VEC2I:
    as_vec2i = _argument.as_vec2i;
    break;
  }
}

Command::Command(Memory::Allocator& _allocator, const String& _name,
                 const char* _signature, Delegate&& _function)
  : m_allocator{&_allocator}
  , m_delegate{Utility::move(_function)}
  , m_arguments{allocator()}
  , m_declaration{allocator()}
  , m_name{allocator(), _name}
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
        m_declaration += String::format("vec%c%c", dims_ch, type_ch);
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

bool Command::execute(Context& console_) {
  // Check the signature of the arguments
  if (m_arguments.size() != m_argument_count) {
    logger->error(
      "arity violation in call, expected %zu parameters, got %zu",
      m_argument_count,
      m_arguments.size());
    return false;
  }

  const Argument* arg = m_arguments.data();
  const char* expected = "";
  for (const char* ch = m_signature; *ch; ch++, arg++) {
    switch (*ch) {
    case 'b':
      if (arg->type != VariableType::BOOLEAN) {
        expected = "bool";
        goto error;
      }
      break;
    case 's':
      if (arg->type != VariableType::STRING) {
        expected = "string";
        goto error;
      }
      break;
    case 'i':
      if (arg->type != VariableType::INT) {
        expected = "int";
        goto error;
      }
      break;
    case 'f':
      if (arg->type != VariableType::FLOAT) {
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
          if (*ch == 'f' && arg->type != VariableType::VEC2F) {
            expected = "vec2f";
            goto error;
          } else if (*ch == 'i' && arg->type != VariableType::VEC2I) {
            expected = "vec2i";
            goto error;
          }
          break;
        case '3':
          if (*ch == 'f' && arg->type != VariableType::VEC3F) {
            expected = "vec3f";
            goto error;
          } else if (*ch == 'i' && arg->type != VariableType::VEC3I) {
            expected = "vec3i";
            goto error;
          }
          break;
        case '4':
          if (*ch == 'f' && arg->type != VariableType::VEC4F) {
            expected = "vec4f";
            goto error;
          } else if (*ch == 'i' && arg->type != VariableType::VEC4I) {
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

  return m_delegate(console_, m_arguments);

error:
  logger->error(
    "%s: expected '%s' for argument %zu, got '%s' instead",
    m_declaration,
    expected,
    static_cast<Size>((arg - &m_arguments.first()) + 1),
    variable_type_string(arg->type));
  return false;
}

bool Command::execute_tokens(Context& console_, const Vector<Token>& _tokens) {
  m_arguments.clear();

  const auto result = _tokens.each_fwd([&](const Token& _token) {
    switch (_token.kind()) {
    case Token::Type::ATOM:
      if (!m_arguments.emplace_back(_token.as_atom())) return false;
      break;
    case Token::Type::STRING:
      if (!m_arguments.emplace_back(_token.as_string())) return false;
      break;
    case Token::Type::BOOLEAN:
      if (!m_arguments.emplace_back(_token.as_boolean())) return false;
      break;
    case Token::Type::INT:
      if (!m_arguments.emplace_back(_token.as_int())) return false;
      break;
    case Token::Type::FLOAT:
      if (!m_arguments.emplace_back(_token.as_float())) return false;
      break;
    case Token::Type::VEC4F:
      if (!m_arguments.emplace_back(_token.as_vec4f())) return false;
      break;
    case Token::Type::VEC4I:
      if (!m_arguments.emplace_back(_token.as_vec4i())) return false;
      break;
    case Token::Type::VEC3F:
      if (!m_arguments.emplace_back(_token.as_vec3f())) return false;
      break;
    case Token::Type::VEC3I:
      if (!m_arguments.emplace_back(_token.as_vec3i())) return false;
      break;
    case Token::Type::VEC2F:
      if (!m_arguments.emplace_back(_token.as_vec2f())) return false;
      break;
    case Token::Type::VEC2I:
      if (!m_arguments.emplace_back(_token.as_vec2i())) return false;
      break;
    }
  });

  return result && execute(console_);
}

} // namespace rx::console
