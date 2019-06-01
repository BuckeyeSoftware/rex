#ifndef RX_CORE_JSON_H
#define RX_CORE_JSON_H

#include "rx/core/string.h"
#include "rx/core/optional.h"

#include "rx/core/memory/allocator.h"
#include "rx/core/memory/system_allocator.h"

#include "core/traits/return_type.h"
#include "core/traits/is_same.h"

#include "lib/json.h"

namespace rx {

struct json {
  constexpr json();
  json(const string& _contents);
  json(memory::allocator* _allocator, const string& _contents);
  ~json();

  enum class type {
    k_array,
    k_boolean,
    k_null,
    k_number,
    k_object,
    k_string,
    k_integer
  };

  operator bool() const;
  optional<string> error() const;

  bool is_type(type _type) const;

  bool is_array() const;
  bool is_array_of(type _type) const;
  bool is_array_of(type _type, rx_size _size) const;
  bool is_boolean() const;
  bool is_null() const;
  bool is_number() const;
  bool is_object() const;
  bool is_string() const;
  bool is_integer() const;

  json operator[](rx_size _index) const;
  bool as_boolean() const;
  rx_f64 as_number() const;
  rx_f32 as_float() const;
  rx_s32 as_integer() const;
  json operator[](const char* _name) const;
  string as_string() const;

  // # of elements for objects and arrays only
  rx_size size() const;

  template<typename F>
  bool each(F&& _function) const;

private:
  json(struct json_value_s* _head);

  memory::allocator* m_allocator;
  struct json_value_s* m_root;
  struct json_parse_result_s m_error;
};

inline constexpr json::json()
  : m_allocator{nullptr}
  , m_root{nullptr}
  , m_error{}
{
}

inline json::json(const string& _contents)
  : json{&memory::g_system_allocator, _contents}
{
}

inline json::operator bool() const {
  return m_root;
}

inline bool json::is_array() const {
  return is_type(type::k_array);
}

inline bool json::is_array_of(type _type) const {
  if (!is_array()) {
    return false;
  }

  return each([_type](const json& _value) {
    return _value.is_type(_type);
  });
}

inline bool json::is_array_of(type _type, rx_size _size) const {
  if (!is_array()) {
    return false;
  }

  if (size() != _size) {
    return false;
  }

  return each([_type](const json& _value) {
    return _value.is_type(_type);
  });
}

inline bool json::is_boolean() const {
  return is_type(type::k_boolean);
}

inline bool json::is_null() const {
  return is_type(type::k_null);
}

inline bool json::is_number() const {
  return is_type(type::k_number);
}

inline bool json::is_object() const {
  return is_type(type::k_object);
}

inline bool json::is_string() const {
  return is_type(type::k_string);
}

inline bool json::is_integer() const {
  return is_type(type::k_integer);
}

template<typename F>
inline bool json::each(F&& _function) const {
  for (rx_size i{0}; i < size(); i++) {
    if constexpr(traits::is_same<traits::return_type<F>, bool>) {
      if (!_function(operator[](i))) {
        return false;
      }
    } else {
      _function(operator[](i));
    }
  }
  return true;
}

} // namespace rx

#endif // RX_CORE_JSON_H