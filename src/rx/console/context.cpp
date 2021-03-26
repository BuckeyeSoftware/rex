#include <string.h> // strcmp
#include <stdlib.h> // strtoll, strtof
#include <errno.h> // errno, ERANGE

#include "rx/console/context.h"
#include "rx/console/variable.h"
#include "rx/console/command.h"
#include "rx/console/parser.h"

#include "rx/core/concurrency/spin_lock.h"
#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/log.h" // RX_LOG

namespace Rx::Console {

static Concurrency::SpinLock g_lock;

static VariableReference* g_head RX_HINT_GUARDED_BY(g_lock);

RX_LOG("console", logger);

static GlobalGroup g_group_cvars{"console"};

static bool type_check(VariableType _VariableType, Token::Type _token_type) {
  switch (_VariableType) {
  case VariableType::BOOLEAN:
    return _token_type == Token::Type::BOOLEAN;
  case VariableType::STRING:
    return _token_type == Token::Type::STRING;
  case VariableType::INT:
    return _token_type == Token::Type::INT;
  case VariableType::FLOAT:
    return _token_type == Token::Type::FLOAT;
  case VariableType::VEC4F:
    return _token_type == Token::Type::VEC4F;
  case VariableType::VEC4I:
    return _token_type == Token::Type::VEC4I;
  case VariableType::VEC3F:
    return _token_type == Token::Type::VEC3F;
  case VariableType::VEC3I:
    return _token_type == Token::Type::VEC3I;
  case VariableType::VEC2F:
    return _token_type == Token::Type::VEC2F;
  case VariableType::VEC2I:
    return _token_type == Token::Type::VEC2I;
  }
  return false;
}

bool Context::write(const String& message_) {
  return m_lines.push_back(message_);
}

void Context::clear() {
  m_lines.clear();
}

const Vector<String>& Context::lines() {
  return m_lines;
}

Command* Context::add_command(const String& _name, const char* _signature,
  Function<bool(Context& console_, const Vector<Command::Argument>&)>&& _function)
{
  // Don't allow adding the same command multiple times.
  if (m_commands.find(_name)) {
    return nullptr;
  }

  return m_commands.insert(_name, {_name, _signature, Utility::move(_function)});
}

bool Context::execute(const String& _contents) {
  Parser parse{Memory::SystemAllocator::instance()};

  if (!parse.parse(_contents)) {
    const auto& diagnostic{parse.error()};

    print("^rerror: ^w%s", diagnostic.message);
    print("%s", _contents);

    String format;
    format += String::format("%*s^r", static_cast<int>(diagnostic.offset), "");
    if (!diagnostic.inside) {
      for (Size i = 0; i < diagnostic.length; i++) {
        format += '~';
      }
    }

    if (diagnostic.caret) {
      format += "^^";
    }

    print("%s", format);

    return false;
  }

  auto tokens = Utility::move(parse.tokens());

  if (tokens.is_empty()) {
    return false;
  }

  if (tokens[0].kind() != Token::Type::ATOM) {
    print("^rerror: ^wexpected atom");
    return false;
  }

  const auto& atom = tokens[0].as_atom();
  if (auto* variable = find_variable_by_name(atom)) {
    if (tokens.size() > 1) {
      switch (set_from_reference_and_token(variable, tokens[1])) {
      case VariableStatus::SUCCESS:
        print("^gsuccess: ^wChanged: \"%s\" to %s", atom, tokens[1].print());
        break;
      case VariableStatus::OUT_OF_RANGE:
        print("^rerror: ^wOut of range: \"%s\" has range %s", atom,
          variable->print_range());
        break;
      case VariableStatus::TYPE_MISMATCH:
        print("^rerror: ^wType mismatch: \"%s\" expected %s, got %s", atom,
          VariableType_as_string(variable->type()),
          token_type_as_string(tokens[1].kind()));
        break;
      }
    } else {
      print("^cinfo: ^w%s = %s", atom, variable->print_current());
    }
  } else if (auto* command = m_commands.find(atom)) {
    tokens.erase(0, 1);
    if (!command->execute_tokens(*this, tokens)) {
      return false;
    }
  } else {
    print("^rerror: ^wCommand or variable \"%s\", not found", atom);
  }

  return true;
}

Optional<Vector<String>> Context::auto_complete_variables(const String& _prefix) {
  Vector<String> results;
  for (VariableReference* node = g_head; node; node = node->m_next) {
    if (!strncmp(node->name(), _prefix.data(), _prefix.size())) {
      if (!results.push_back(node->name())) {
        return nullopt;
      }
    }
  }
  return results;
}

Optional<Vector<String>> Context::auto_complete_commands(const String& _prefix) {
  Vector<String> results;
  auto result = m_commands.each_key([&](const String& _key) {
    if (!strncmp(_key.data(), _prefix.data(), _prefix.size())) {
      if (!results.push_back(_key)) {
        return false;
      }
    }
    return true;
  });

  if (result) {
    return results;
  }

  return nullopt;
}

bool Context::load(const char* _file_name) {
  // sort references
  {
    Concurrency::ScopeLock locked{g_lock};
    g_head = sort(g_head);
  }

  auto& allocator = Memory::SystemAllocator::instance();

  auto data = Filesystem::read_text_file(allocator, _file_name);
  if (!data) {
    return false;
  }

  auto next_line = [&](char*& line_) -> char* {
    if (!line_ || !*line_) {
      return nullptr;
    }
    char* line = line_;
    line_ += strcspn(line_, "\n");
    *line_++ = '\0';
    return line;
  };

  Parser parse{allocator};

  auto token = reinterpret_cast<char*>(data->data());
  while (auto line = next_line(token)) {
    // Skip whitespace.
    while (strchr(" \t", *line)) {
      line++;
    }

    // Ignore empty and comment lines.
    if (!*line || strchr("#;[", *line)) {
      continue;
    }

    if (!parse.parse(line)) {
      logger->error("%s", parse.error().message);
      continue;
    }

    auto tokens = Utility::move(parse.tokens());
    if (tokens.size() < 2 || tokens[0].kind() != Token::Type::ATOM) {
      continue;
    }

    const auto& atom = tokens[0].as_atom();
    if (auto variable = find_variable_by_name(atom)) {
      set_from_reference_and_token(variable, tokens[1]);
    } else {
      logger->error("'%s' not found", atom);
    }
  }

  return true;
}

bool Context::save(const char* _file_name) {
  auto file = Filesystem::File::open(m_allocator, _file_name, "w");
  if (!file) {
    return false;
  }

  #define attempt(_expr) \
    if (!(_expr)) return false

  logger->info("saving '%s'", _file_name);
  for (const VariableReference *head = g_head; head; head = head->m_next) {
    if (VariableType_is_ranged(head->type())) {
      attempt(file->print(m_allocator, "## %s (in range %s, defaults to %s)\n",
        head->description(), head->print_range(), head->print_initial()));
      attempt(file->print(m_allocator, head->is_initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), head->print_current()));
    } else {
      attempt(file->print(m_allocator, "## %s (defaults to %s)\n",
        head->description(), head->print_initial()));
      attempt(file->print(m_allocator, head->is_initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), head->print_current()));
    }
  }

  #undef attempt

  return true;
}

template<typename T>
VariableStatus Context::set_from_reference_and_value(VariableReference* _reference, const T& _value) {
  if (auto* cast{_reference->try_cast<T>()}) {
    return cast->set(_value);
  } else {
    return VariableStatus::TYPE_MISMATCH;
  }
}

template VariableStatus Context::set_from_reference_and_value<bool>(VariableReference* _reference, const bool& _value);
template VariableStatus Context::set_from_reference_and_value<String>(VariableReference* _reference, const String& _value);
template VariableStatus Context::set_from_reference_and_value<Sint32>(VariableReference* _reference, const Sint32& _value);
template VariableStatus Context::set_from_reference_and_value<Float32>(VariableReference* _reference, const Float32& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec4f>(VariableReference* _reference, const Math::Vec4f& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec4i>(VariableReference* _reference, const Math::Vec4i& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec3f>(VariableReference* _reference, const Math::Vec3f& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec3i>(VariableReference* _reference, const Math::Vec3i& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec2f>(VariableReference* _reference, const Math::Vec2f& _value);
template VariableStatus Context::set_from_reference_and_value<Math::Vec2i>(VariableReference* _reference, const Math::Vec2i& _value);

VariableStatus Context::set_from_reference_and_token(VariableReference* reference_, const Token& _token) {
  if (!type_check(reference_->type(), _token.kind())) {
    return VariableStatus::TYPE_MISMATCH;
  }

  switch (reference_->type()) {
  case VariableType::BOOLEAN:
    return reference_->cast<Bool>()->set(_token.as_boolean());
  case VariableType::STRING:
    return reference_->cast<String>()->set(_token.as_string());
  case VariableType::INT:
    return reference_->cast<Sint32>()->set(_token.as_int());
  case VariableType::FLOAT:
    return reference_->cast<Float32>()->set(_token.as_float());
  case VariableType::VEC4F:
    return reference_->cast<Math::Vec4f>()->set(_token.as_vec4f());
  case VariableType::VEC4I:
    return reference_->cast<Math::Vec4i>()->set(_token.as_vec4i());
  case VariableType::VEC3F:
    return reference_->cast<Math::Vec3f>()->set(_token.as_vec3f());
  case VariableType::VEC3I:
    return reference_->cast<Math::Vec3i>()->set(_token.as_vec3i());
  case VariableType::VEC2F:
    return reference_->cast<Math::Vec2f>()->set(_token.as_vec2f());
  case VariableType::VEC2I:
    return reference_->cast<Math::Vec2i>()->set(_token.as_vec2i());
  }

  RX_HINT_UNREACHABLE();
}

VariableReference* Context::find_variable_by_name(const char* _name) {
  for (VariableReference* head{g_head}; head; head = head->m_next) {
    if (!strcmp(head->name(), _name)) {
      return head;
    }
  }
  return nullptr;
}

VariableReference* Context::add_variable(VariableReference* reference) {
  logger->info("registered '%s'", reference->m_name);
  Concurrency::ScopeLock locked(g_lock);
  VariableReference* next = g_head;
  g_head = reference;
  return next;
}

VariableReference* Context::split(VariableReference* reference) {
  if (!reference || !reference->m_next) {
    return nullptr;
  }
  VariableReference* splitted = reference->m_next;
  reference->m_next = splitted->m_next;
  splitted->m_next = split(splitted->m_next);
  return splitted;
}

VariableReference* Context::merge(VariableReference* lhs, VariableReference* rhs) {
  if (!lhs) {
    return rhs;
  }

  if (!rhs) {
    return lhs;
  }

  if (strcmp(lhs->m_name, rhs->m_name) > 0) {
    rhs->m_next = merge(lhs, rhs->m_next);
    return rhs;
  }

  lhs->m_next = merge(lhs->m_next, rhs);
  return lhs;
}

VariableReference* Context::sort(VariableReference* reference) {
  if (!reference) {
    return nullptr;
  }

  if (!reference->m_next) {
    return reference;
  }

  VariableReference* splitted = split(reference);

  return merge(sort(reference), sort(splitted));
}

} // namespace Rx::Console
