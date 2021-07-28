#include <stdlib.h> // strtod
#include <string.h> // strcmp

#include "rx/core/serialize/json.h"
#include "rx/core/concurrency/atomic.h"

#include "rx/core/math/floor.h"

#include "rx/core/hints/unreachable.h"

#include "lib/json.h"

namespace Rx::Serialize {

static const char* json_parse_error_to_string(enum json_parse_error_e _error) {
  switch (_error) {
  case json_parse_error_expected_comma_or_closing_bracket:
    return "expected a comma, closing '}', or ']'";
  case json_parse_error_expected_colon:
    return "expected a colon";
  case json_parse_error_expected_opening_quote:
    return "expected opening quote '\"'";
  case json_parse_error_invalid_string_escape_sequence:
    return "invalid string escape sequence";
  case json_parse_error_invalid_number_format:
    return "invalid number formatting";
  case json_parse_error_invalid_value:
    return "invalid value";
  case json_parse_error_premature_end_of_buffer:
    return "premature end of buffer";
  case json_parse_error_invalid_string:
    return "malformed string";
  case json_parse_error_allocator_failed:
    return "out of memory";
  case json_parse_error_unexpected_trailing_characters:
    return "unexpected trailing characters";
  case json_parse_error_unknown:
    [[fallthrough]];
  default:
    return "unknown error";
  }
}

static void* json_allocator(void* _user, Size _size) {
  auto allocator{reinterpret_cast<Memory::Allocator*>(_user)};
  return allocator->allocate(_size);
}

struct JSON::Shared {
  Shared(Memory::Allocator& _allocator, const StringView& _contents);
  ~Shared();

  // Shared* acquire();
  // void release();

  Memory::Allocator& allocator;
  json_value_s* root;
  json_parse_result_s error;
  Concurrency::Atomic<Size> count;
};

JSON::Shared::Shared(Memory::Allocator& _allocator, const StringView& _contents)
  : allocator{_allocator}
  , root{nullptr}
  , count{0}
{
  root = json_parse_ex(_contents.data(), _contents.size(),
    (json_parse_flags_allow_c_style_comments |
     json_parse_flags_allow_location_information |
     json_parse_flags_allow_unquoted_keys |
     json_parse_flags_allow_multi_line_strings),
    json_allocator,
    &allocator,
    &error);
}

JSON::Shared::~Shared() {
  allocator.deallocate(root);
}

Optional<JSON> JSON::parse(Memory::Allocator& _allocator, const StringView& _contents) {
  auto shared = _allocator.create<Shared>(_allocator, _contents);
  if (!shared) {
    return nullopt;
  }
  return JSON { shared, shared->root };
}

JSON::JSON(Shared* _shared, const void* _value)
  : m_shared{acquire(_shared)}
  , m_value{_value}
{
}

Optional<String> JSON::error() const {
  if (!m_shared) {
    return nullopt;
  }

  const auto code =
    static_cast<enum json_parse_error_e>(m_shared->error.error);

  return String::format(m_shared->allocator, "%zu:%zu %s",
    m_shared->error.error_line_no, m_shared->error.error_offset,
    json_parse_error_to_string(code));
}

bool JSON::is_type(Type _type) const {
  const auto value = static_cast<const json_value_s*>(m_value);
  switch (_type) {
  case Type::ARRAY:
    return value->type == json_type_array;
  case Type::BOOLEAN:
    return value->type == json_type_true || value->type == json_type_false;
  case Type::INTEGER:
    return value->type == json_type_number && Math::floor(as_number()) == as_number();
  case Type::NIL:
    return !m_value || value->type == json_type_null;
  case Type::NUMBER:
    return value->type == json_type_number;
  case Type::OBJECT:
    return value->type == json_type_object;
  case Type::STRING:
    return value->type == json_type_string;
  }

  RX_HINT_UNREACHABLE();
}

JSON JSON::operator[](Size _index) const {
  RX_ASSERT(is_array() || is_object(), "not an indexable type");

  const auto value = static_cast<const json_value_s*>(m_value);

  if (is_array()) {
    const auto array = reinterpret_cast<const json_array_s*>(value->payload);
    auto element = array->start;
    RX_ASSERT(_index < array->length, "out of bounds");
    for (Size i = 0; i < _index; i++) {
      element = element->next;
    }
    return {m_shared, element->value};
  } else {
    const auto object = reinterpret_cast<const json_object_s*>(value->payload);
    auto element = object->start;
    RX_ASSERT(_index < object->length, "out of bounds");
    for (Size i = 0; i < _index; i++) {
      element = element->next;
    }
    return {m_shared, element->value};
  }

  return {};
}

bool JSON::as_boolean() const {
  RX_ASSERT(is_boolean(), "not a boolean");
  return static_cast<const json_value_s*>(m_value)->type == json_type_true;
}

Float64 JSON::as_number() const {
  RX_ASSERT(is_number(), "not a number");
  const auto value = static_cast<const json_value_s*>(m_value);
  const auto number = static_cast<const json_number_s*>(value->payload);
  return strtod(number->number, nullptr);
}

Float32 JSON::as_float() const {
  return static_cast<Float32>(as_number());
}

Sint32 JSON::as_integer() const {
  RX_ASSERT(is_integer(), "not a integer");
  return static_cast<Sint32>(as_number());
}

JSON JSON::operator[](const char* _name) const {
  RX_ASSERT(is_object(), "not a object");
  const auto value = static_cast<const json_value_s*>(m_value);
  auto object = static_cast<const json_object_s*>(value->payload);
  for (auto element = object->start; element; element = element->next) {
    if (!strcmp(element->name->string, _name)) {
      return {m_shared, element->value};
    }
  }
  return {};
}

Optional<String> JSON::as_string(Memory::Allocator& _allocator) const {
  RX_ASSERT(is_string(), "not a string");
  const auto value = static_cast<const json_value_s*>(m_value);
  const auto string = static_cast<const json_string_s*>(value->payload);
  return String::create(_allocator, string->string, string->string_size);
}

Size JSON::size() const {
  RX_ASSERT(is_array() || is_object(), "not an indexable type");
  const auto value = static_cast<const json_value_s*>(m_value);
  switch (value->type) {
  case json_type_array:
    return static_cast<const json_array_s*>(value->payload)->length;
  case json_type_object:
    return static_cast<const json_object_s*>(value->payload)->length;
  }
  return 0;
}

JSON::Shared* JSON::acquire(Shared* _shared) {
  _shared->count.fetch_add(1, Concurrency::MemoryOrder::RELAXED);
  return _shared;
}

void JSON::release(Shared* _shared){
  if (_shared) {
    if (_shared->count.fetch_sub(1, Concurrency::MemoryOrder::ACQ_REL) == 1) {
      _shared->allocator.destroy<Shared>(_shared);
    }
  }
}

bool JSON::is_valid(const Shared* _shared) {
  return _shared && _shared->root;
}

bool JSON::enumerate(const void* _user, const EnumerateCallback& _callback) const {
  const auto value = static_cast<const json_value_s*>(m_value);
  const auto type = value->type;
  if (type == json_type_array) {
    const auto payload = static_cast<const json_array_s*>(value->payload);
    for (auto element = payload->start; element; element = element->next) {
      if (!_callback(m_shared, _user, element->value)) {
        return false;
      }
    }
  } else if (type == json_type_object) {
    const auto payload = static_cast<const json_object_s*>(value->payload);
    for (auto element = payload->start; element; element = element->next) {
      if (!_callback(m_shared, _user, element->value)) {
        return false;
      }
    }
  } else {
    return false;
  }
  return true;
}

} // namespace Rx