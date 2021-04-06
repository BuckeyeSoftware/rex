#ifndef RX_CORE_JSON_H
#define RX_CORE_JSON_H
#include "rx/core/concurrency/atomic.h"

#include "rx/core/traits/invoke_result.h"
#include "rx/core/traits/is_same.h"

#include "rx/core/utility/declval.h"
#include "rx/core/utility/exchange.h"

#include "rx/core/string.h"
#include "rx/core/optional.h"

#include "lib/json.h"

namespace Rx {

// 32-bit: 8 bytes
// 64-bit: 16 bytes
struct RX_API JSON {
  constexpr JSON();
  JSON(const JSON& _json);
  JSON(JSON&& json_);
  ~JSON();

  static Optional<JSON> parse(Memory::Allocator& _allocator, const char* _contents, Size _length);
  static Optional<JSON> parse(Memory::Allocator& _allocator, const char* _contents);
  static Optional<JSON> parse(Memory::Allocator& _allocator, const String& _contents);

  JSON& operator=(const JSON& _json);
  JSON& operator=(JSON&& json_);

  enum class Type : Uint8 {
    ARRAY,
    BOOLEAN,
    NIL,
    NUMBER,
    OBJECT,
    STRING,
    INTEGER
  };

  operator bool() const;
  Optional<String> error() const;

  bool is_type(Type _type) const;

  bool is_array() const;
  bool is_array_of(Type _type) const;
  bool is_array_of(Type _type, Size _size) const;
  bool is_boolean() const;
  bool is_null() const;
  bool is_number() const;
  bool is_object() const;
  bool is_string() const;
  bool is_integer() const;

  JSON operator[](Size _index) const;
  bool as_boolean() const;
  Float64 as_number() const;
  Float32 as_float() const;
  Sint32 as_integer() const;
  JSON operator[](const char* _name) const;
  String as_string() const;
  String as_string_with_allocator(Memory::Allocator& _allocator) const;

  // # of elements for objects and arrays only
  Size size() const;
  bool is_empty() const;

  template<typename F>
  bool each(F&& _function) const;

  constexpr Memory::Allocator& allocator() const;

private:
  struct Shared {
    Shared(Memory::Allocator& _allocator, const char* _contents, Size _length);
    ~Shared();

    Shared* acquire();
    void release();

    Memory::Allocator& m_allocator;
    struct json_parse_result_s m_error;
    struct json_value_s* m_root;
    Concurrency::Atomic<Size> m_count;
  };

  JSON(Shared* _shared, struct json_value_s* _head);

  Shared* m_shared;
  struct json_value_s* m_value;
};

inline constexpr JSON::JSON()
  : m_shared{nullptr}
  , m_value{nullptr}
{
}

inline Optional<JSON> JSON::parse(Memory::Allocator& _allocator, const String& _contents) {
  return parse(_allocator, _contents.data(), _contents.size());
}

inline JSON::JSON(const JSON& _json)
  : m_shared{_json.m_shared->acquire()}
  , m_value{_json.m_value}
{
}

inline JSON::JSON(JSON&& json_)
  : m_shared{Utility::exchange(json_.m_shared, nullptr)}
  , m_value{Utility::exchange(json_.m_value, nullptr)}
{
}

inline JSON::~JSON() {
  if (m_shared) {
    m_shared->release();
  }
}

inline JSON& JSON::operator=(const JSON& _json) {
  if (&_json != this) {
    if (m_shared) {
      m_shared->release();
    }
    m_shared = _json.m_shared->acquire();
    m_value = _json.m_value;
  }
  return *this;
}

inline JSON& JSON::operator=(JSON&& json_) {
  if (&json_ != this) {
    m_shared = Utility::exchange(json_.m_shared, nullptr);
    m_value = Utility::exchange(json_.m_value, nullptr);
  }
  return *this;
}

inline JSON::operator bool() const {
  return m_shared && m_shared->m_root;
}

inline bool JSON::is_array() const {
  return is_type(Type::ARRAY);
}

inline bool JSON::is_array_of(Type _type) const {
  if (!is_array()) {
    return false;
  }

  return each([_type](const JSON& _value) {
    return _value.is_type(_type);
  });
}

inline bool JSON::is_array_of(Type _type, Size _size) const {
  if (!is_array()) {
    return false;
  }

  if (size() != _size) {
    return false;
  }

  return each([_type](const JSON& _value) {
    return _value.is_type(_type);
  });
}

inline bool JSON::is_boolean() const {
  return is_type(Type::BOOLEAN);
}

inline bool JSON::is_null() const {
  return is_type(Type::NIL);
}

inline bool JSON::is_number() const {
  return is_type(Type::NUMBER);
}

inline bool JSON::is_object() const {
  return is_type(Type::OBJECT);
}

inline bool JSON::is_string() const {
  return is_type(Type::STRING);
}

inline bool JSON::is_integer() const {
  return is_type(Type::INTEGER);
}

inline bool JSON::is_empty() const {
  return size() == 0;
}

inline String JSON::as_string() const {
  return as_string_with_allocator(allocator());
}

template<typename F>
bool JSON::each(F&& _function) const {
  const bool array = is_array();
  const bool object = is_object();

  RX_ASSERT(array || object, "not enumerable");

  using ReturnType = Traits::InvokeResult<F, const JSON&>;

  if (array) {
    auto array = reinterpret_cast<struct json_array_s*>(m_value->payload);
    for (auto element = array->start; element; element = element->next) {
      if constexpr(Traits::IS_SAME<ReturnType, bool>) {
        if (!_function({m_shared, element->value})) {
          return false;
        }
      } else {
        _function({m_shared, element->value});
      }
    }
  } else {
    auto object = reinterpret_cast<struct json_object_s*>(m_value->payload);
    for (auto element = object->start; element; element = element->next) {
      if constexpr(Traits::IS_SAME<ReturnType, bool>) {
        if (!_function({m_shared, element->value})) {
          return false;
        }
      } else {
        _function({m_shared, element->value});
      }
    }
  }

  return true;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& JSON::allocator() const {
  RX_ASSERT(m_shared, "reference count reached zero");
  return m_shared->m_allocator;
}

} // namespace Rx

#endif // RX_CORE_JSON_H
