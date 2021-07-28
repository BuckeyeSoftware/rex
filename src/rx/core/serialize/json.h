#ifndef RX_CORE_SERIALIZE_JSON_H
#define RX_CORE_SERIALIZE_JSON_H
#include "rx/core/optional.h"
#include "rx/core/string.h"

namespace Rx::Serialize {

// 32-bit: 8 bytes
// 64-bit: 16 bytes
struct RX_API JSON {
  constexpr JSON();
  JSON(const JSON& _json);
  JSON(JSON&& json_);
  ~JSON();

  static Optional<JSON> parse(Memory::Allocator& _allocator, const StringView& _contents);

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

  explicit operator bool() const;
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
  Optional<String> as_string(Memory::Allocator& _allocator) const;

  // # of elements for objects and arrays only
  Size size() const;
  bool is_empty() const;

  template<typename F>
  bool each(F&& _function) const;

private:
  struct Shared;

  using EnumerateCallback = bool(*)(Shared*, const void*, const void*);

  JSON(Shared* _shared, const void* _head);

  bool enumerate(const void* _user, const EnumerateCallback& _callback) const;

  // Shared state reference count helpers.
  static Shared* acquire(Shared* _shared);
  static void release(Shared* _shared);

  // Check if the shared state is valid or not.
  static bool is_valid(const Shared* _shared);

  Shared* m_shared;
  const void* m_value;
};

inline constexpr JSON::JSON()
  : m_shared{nullptr}
  , m_value{nullptr}
{
}

inline JSON::JSON(const JSON& _json)
  : m_shared{acquire(_json.m_shared)}
  , m_value{_json.m_value}
{
}

inline JSON::JSON(JSON&& json_)
  : m_shared{Utility::exchange(json_.m_shared, nullptr)}
  , m_value{Utility::exchange(json_.m_value, nullptr)}
{
}

inline JSON::~JSON() {
  release(m_shared);
}

inline JSON& JSON::operator=(const JSON& _json) {
  if (&_json != this) {
    release(m_shared);
    m_shared = acquire(_json.m_shared);
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
  return is_valid(m_shared);
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

template<typename F>
RX_HINT_FORCE_INLINE bool JSON::each(F&& _function) const {
  using ReturnType = Traits::InvokeResult<F, const JSON&>;
  return enumerate(
    static_cast<const void*>(&_function),
    [](Shared* _shared, const void* _callable, const void* _value) {
      const auto& function = *static_cast<const F*>(_callable);
      if constexpr (Traits::IS_SAME<ReturnType, bool>) {
        return function({_shared, _value});
      } else {
        function({_shared, _value});
      }
      return true;
    });
}

} // namespace Rx

#endif // RX_CORE_JSON_H