#include <string.h> // strcmp
#include <stdlib.h> // strtoll, strtof
#include <errno.h> // errno, ERANGE

#include "rx/console/interface.h"
#include "rx/console/variable.h"
#include "rx/console/command.h"
#include "rx/console/parser.h"

#include "rx/core/concurrency/spin_lock.h"
#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/map.h"

#include "rx/core/log.h" // RX_LOG

namespace rx::console {

static concurrency::spin_lock g_lock;
static variable_reference* g_head; // protected by |g_lock|

RX_LOG("console", logger);

static global_group g_group_cvars{"cvars"};
static global_group g_group_console{"console"};

// TODO(dweiler): limited line count queue for messages on the console.
static global<vector<string>> g_lines{"console", "lines"};
static global<map<string, command>> g_commands{"console", "commands"};

void interface::write(const string& message_) {
  g_lines->push_back(message_);
}

void interface::clear() {
  g_lines->clear();
}

const vector<string>& interface::lines() {
  return *g_lines;
}

void interface::add_command(const string& _name, const char* _signature,
  function<bool(const vector<command::argument>&)>&& _function)
{
  g_commands->insert(_name, {_name, _signature, utility::move(_function)});
}

static bool type_check(variable_type _variable_type, token::type _token_type) {
  switch (_variable_type) {
  case variable_type::k_boolean:
    return _token_type == token::type::k_boolean;
  case variable_type::k_string:
    return _token_type == token::type::k_string;
  case variable_type::k_int:
    return _token_type == token::type::k_int;
  case variable_type::k_float:
    return _token_type == token::type::k_float;
  case variable_type::k_vec4f:
    return _token_type == token::type::k_vec4f;
  case variable_type::k_vec4i:
    return _token_type == token::type::k_vec4i;
  case variable_type::k_vec3f:
    return _token_type == token::type::k_vec3f;
  case variable_type::k_vec3i:
    return _token_type == token::type::k_vec3i;
  case variable_type::k_vec2f:
    return _token_type == token::type::k_vec2f;
  case variable_type::k_vec2i:
    return _token_type == token::type::k_vec2i;
  }
  return false;
}

bool interface::execute(const string& _contents) {
  parser parse{memory::system_allocator::instance()};

  if (!parse.parse(_contents)) {
    const auto& diagnostic{parse.error()};

    print("^rerror: ^w%s", diagnostic.message);
    print("%s", _contents);

    string format;
    format += string::format("%*s^r", static_cast<int>(diagnostic.offset), "");
    if (!diagnostic.inside) {
      for (rx_size i{0}; i < diagnostic.length; i++) {
        format += '~';
      }
    }

    if (diagnostic.caret) {
      format += "^^";
    }

    print("%s", format);

    return false;
  }

  auto tokens{utility::move(parse.tokens())};

  if (tokens.is_empty()) {
    return false;
  }

  if (tokens[0].kind() != token::type::k_atom) {
    print("^rerror: ^wexpected atom");
    return false;
  }

  const auto& atom{tokens[0].as_atom()};
  if (auto* variable{find_variable_by_name(atom)}) {
    if (tokens.size() > 1) {
      switch (set_from_reference_and_token(variable, tokens[1])) {
      case variable_status::k_success:
        print("^gsuccess: ^wChanged: \"%s\" to %s", atom, tokens[1].print());
        break;
      case variable_status::k_out_of_range:
        print("^rerror: ^wOut of range: \"%s\" has range %s", atom,
          variable->print_range());
        break;
      case variable_status::k_type_mismatch:
        print("^rerror: ^wType mismatch: \"%s\" expected %s, got %s", atom,
          variable_type_as_string(variable->type()),
          token_type_as_string(tokens[1].kind()));
        break;
      }
    } else {
      print("^cinfo: ^w%s = %s", atom, variable->print_current());
    }
  } else if (auto* command{g_commands->find(atom)}) {
    tokens.erase(0, 1);
    command->execute_tokens(tokens);
  } else {
    print("^rerror: ^wCommand or variable \"%s\", not found", atom);
  }

  return true;
}

vector<string> interface::auto_complete_variables(const string& _prefix) {
  vector<string> results;
  for (variable_reference* node{g_head}; node; node = node->m_next) {
    if (!strncmp(node->m_name, _prefix.data(), _prefix.size())) {
      results.push_back(node->m_name);
    }
  }
  return results;
}

vector<string> interface::auto_complete_commands(const string& _prefix) {
  vector<string> results;
  g_commands->each_key([&](const string& _key) {
    if (!strncmp(_key.data(), _prefix.data(), _prefix.size())) {
      results.push_back(_key);
    }
  });
  return results;
}

bool interface::load(const char* file_name) {
  // sort references
  {
    concurrency::scope_lock locked(g_lock);
    g_head = sort(g_head);
  }

  filesystem::file file(file_name, "r");
  if (!file) {
    return false;
  }

  logger->info("loading '%s'", file_name);

  parser parse{memory::system_allocator::instance()};
  for (string line_contents; file.read_line(line_contents); ) {
    string line{line_contents.lstrip(" \t")};
    if (line.is_empty() || strchr("#;[", line[0])) {
      // ignore empty and comment lines
      continue;
    }

    if (!parse.parse(line_contents)) {
      logger->error("%s", parse.error().message);
    } else {
      auto tokens{utility::move(parse.tokens())};

      if (tokens.size() < 2) {
        continue;
      }

      if (tokens[0].kind() != token::type::k_atom) {
        continue;
      }

      const auto& atom{tokens[0].as_atom()};
      if (auto* variable{find_variable_by_name(atom)}) {
        set_from_reference_and_token(variable, tokens[1]);
      } else {
        logger->error("'%s' not found", atom);
      }
    }
  }

  return true;
}

bool interface::save(const char* file_name) {
  filesystem::file file(file_name, "w");
  if (!file) {
    return false;
  }

  logger->info("saving '%s'", file_name);
  for (const variable_reference *head{g_head}; head; head = head->m_next) {
    if (variable_type_is_ranged(head->type())) {
      file.print("## %s (in range %s, defaults to %s)\n",
        head->description(), head->print_range(), head->print_initial());
      file.print(head->is_initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), head->print_current());
    } else {
      file.print("## %s (defaults to %s)\n",
        head->description(), head->print_initial());
      file.print(head->is_initial() ? ";%s %s\n" : "%s %s\n",
        head->name(), head->print_current());
    }
  }

  return true;
}

template<typename T>
variable_status interface::set_from_reference_and_value(variable_reference* _reference, const T& _value) {
  if (auto* cast{_reference->try_cast<T>()}) {
    return cast->set(_value);
  } else {
    return variable_status::k_type_mismatch;
  }
}

template variable_status interface::set_from_reference_and_value<bool>(variable_reference* _reference, const bool& _value);
template variable_status interface::set_from_reference_and_value<string>(variable_reference* _reference, const string& _value);
template variable_status interface::set_from_reference_and_value<rx_s32>(variable_reference* _reference, const rx_s32& _value);
template variable_status interface::set_from_reference_and_value<rx_f32>(variable_reference* _reference, const rx_f32& _value);
template variable_status interface::set_from_reference_and_value<math::vec4f>(variable_reference* _reference, const math::vec4f& _value);
template variable_status interface::set_from_reference_and_value<math::vec4i>(variable_reference* _reference, const math::vec4i& _value);
template variable_status interface::set_from_reference_and_value<math::vec3f>(variable_reference* _reference, const math::vec3f& _value);
template variable_status interface::set_from_reference_and_value<math::vec3i>(variable_reference* _reference, const math::vec3i& _value);
template variable_status interface::set_from_reference_and_value<math::vec2f>(variable_reference* _reference, const math::vec2f& _value);
template variable_status interface::set_from_reference_and_value<math::vec2i>(variable_reference* _reference, const math::vec2i& _value);

variable_status interface::set_from_reference_and_token(variable_reference* reference_, const token& _token) {
  if (!type_check(reference_->type(), _token.kind())) {
    return variable_status::k_type_mismatch;
  }

  switch (reference_->type()) {
  case variable_type::k_boolean:
    return reference_->cast<bool>()->set(_token.as_boolean());
  case variable_type::k_string:
    return reference_->cast<string>()->set(_token.as_string());
  case variable_type::k_int:
    return reference_->cast<rx_s32>()->set(_token.as_int());
  case variable_type::k_float:
    return reference_->cast<rx_f32>()->set(_token.as_float());
  case variable_type::k_vec4f:
    return reference_->cast<math::vec4f>()->set(_token.as_vec4f());
  case variable_type::k_vec4i:
    return reference_->cast<math::vec4i>()->set(_token.as_vec4i());
  case variable_type::k_vec3f:
    return reference_->cast<math::vec3f>()->set(_token.as_vec3f());
  case variable_type::k_vec3i:
    return reference_->cast<math::vec3i>()->set(_token.as_vec3i());
  case variable_type::k_vec2f:
    return reference_->cast<math::vec2f>()->set(_token.as_vec2f());
  case variable_type::k_vec2i:
    return reference_->cast<math::vec2i>()->set(_token.as_vec2i());
  }

  RX_HINT_UNREACHABLE();
}

variable_reference* interface::find_variable_by_name(const char* _name) {
  for (variable_reference* head{g_head}; head; head = head->m_next) {
    if (!strcmp(head->name(), _name)) {
      return head;
    }
  }
  return nullptr;
}

variable_reference* interface::add_variable(variable_reference* reference) {
  logger->info("registered '%s'", reference->m_name);
  concurrency::scope_lock locked(g_lock);
  variable_reference* next = g_head;
  g_head = reference;
  return next;
}

variable_reference* interface::split(variable_reference* reference) {
  if (!reference || !reference->m_next) {
    return nullptr;
  }
  variable_reference* splitted = reference->m_next;
  reference->m_next = splitted->m_next;
  splitted->m_next = split(splitted->m_next);
  return splitted;
}

variable_reference* interface::merge(variable_reference* lhs, variable_reference* rhs) {
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

variable_reference* interface::sort(variable_reference* reference) {
  if (!reference) {
    return nullptr;
  }

  if (!reference->m_next) {
    return reference;
  }

  variable_reference* splitted = split(reference);

  return merge(sort(reference), sort(splitted));
}

} // namespace rx::console
