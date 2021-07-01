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

// [Command]
Command::Command(Delegate&& delegate_, Vector<Argument>&& arguments_,
  Vector<VariableType>&& signature_, String&& name_)
  : m_delegate{Utility::move(delegate_)}
  , m_arguments{Utility::move(arguments_)}
  , m_signature{Utility::move(signature_)}
  , m_name{Utility::move(name_)}
{
}

Optional<Command> Command::create(Memory::Allocator& _allocator,
  const StringView& _name, const StringView& _signature, Delegate&& delegate_)
{
  auto name = _name.to_string(_allocator);
  if (!name) {
    return nullopt;
  }

  // Consume the signature specification for the command and generate the list.
  Vector<VariableType> signature{_allocator};
  for (const char* ch = _signature.data(); *ch; ch++) switch (*ch) {
  case 'b':
    if (!signature.emplace_back(VariableType::BOOLEAN)) {
      return nullopt;
    }
    break;
  case 'i':
    if (!signature.emplace_back(VariableType::INT)) {
      return nullopt;
    }
    break;
  case 'f':
    if (!signature.emplace_back(VariableType::FLOAT)) {
      return nullopt;
    }
    break;
  case 's':
    if (!signature.emplace_back(VariableType::STRING)) {
      return nullopt;
    }
    break;
  case 'v':
    if (ch[1] >= '2' && ch[1] <= '4' && (ch[2] == 'i' || ch[2] == 'f')) {
      switch (ch[1]) {
      case '2':
        if (!signature.emplace_back(ch[2] == 'i' ? VariableType::VEC2I : VariableType::VEC2F)) {
          return nullopt;
        }
        break;
      case '3':
        if (!signature.emplace_back(ch[2] == 'i' ? VariableType::VEC3I : VariableType::VEC3F)) {
          return nullopt;
        }
        break;
      case '4':
        if (!signature.emplace_back(ch[2] == 'i' ? VariableType::VEC4I : VariableType::VEC4F)) {
          return nullopt;
        }
        break;
      }
      ch += 2;
      break;
    } else {
      return nullopt;
    }
    break;
  default:
    return nullopt;
  }

  return Command {
    Utility::move(delegate_),
    { _allocator },
    Utility::move(signature),
    Utility::move(*name)
  };
}

bool Command::execute(Context& console_) {
  const auto n_arguments = m_arguments.size();

  // Check the signature of the arguments
  if (n_arguments != m_signature.size()) {
    logger->error(
      "arity violation in call, expected %zu parameters, got %zu",
      m_signature.size(),
      n_arguments);
    return false;
  }

  for (Size i = 0; i < n_arguments; i++) {
    if (m_arguments[i].type != m_signature[i]) {
      logger->error(
        "expected '%s' for argument %zu, got '%s' instead",
        variable_type_string(m_signature[i]),
        i + 1,
        variable_type_string(m_arguments[i].type));
      return false;
    }
  }

  return m_delegate(console_, m_arguments);
}

bool Command::execute_tokens(Context& console_, const Vector<Token>& _tokens) {
  m_arguments.clear();

  const auto result = _tokens.each_fwd([&](const Token& _token) {
    switch (_token.kind()) {
    case Token::Type::ATOM:
      return m_arguments.emplace_back(_token.as_atom());
    case Token::Type::STRING:
      return m_arguments.emplace_back(_token.as_string());
    case Token::Type::BOOLEAN:
      return m_arguments.emplace_back(_token.as_boolean());
    case Token::Type::INT:
      return m_arguments.emplace_back(_token.as_int());
    case Token::Type::FLOAT:
      return m_arguments.emplace_back(_token.as_float());
    case Token::Type::VEC4F:
      return m_arguments.emplace_back(_token.as_vec4f());
    case Token::Type::VEC4I:
      return m_arguments.emplace_back(_token.as_vec4i());
    case Token::Type::VEC3F:
      return m_arguments.emplace_back(_token.as_vec3f());
    case Token::Type::VEC3I:
      return m_arguments.emplace_back(_token.as_vec3i());
    case Token::Type::VEC2F:
      return m_arguments.emplace_back(_token.as_vec2f());
    case Token::Type::VEC2I:
      return m_arguments.emplace_back(_token.as_vec2i());
    }
    return false;
  });

  return result && execute(console_);
}

} // namespace Rx::Console
