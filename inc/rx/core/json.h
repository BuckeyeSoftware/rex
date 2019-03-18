#ifndef RX_CORE_JSON_H
#define RX_CORE_JSON_H

#include <rx/core/string.h>
#include <rx/core/optional.h>

#include <rx/core/memory/allocator.h>
#include <rx/core/memory/system_allocator.h>

#include <rx/core/traits/return_type.h>
#include <rx/core/traits/is_same.h>

#include "lib/json.h"

namespace rx {

struct json {
  json(const string& _contents);
  json(memory::allocator* _allocator, const string& _contents);
  ~json();

  operator bool() const;
  optional<string> error() const;

  bool is_array() const;
  bool is_boolean() const;
  bool is_null() const;
  bool is_number() const;
  bool is_object() const;
  bool is_string() const;

  json operator[](rx_size _index) const;
  bool as_boolean() const;
  rx_f64 as_number() const;
  json operator[](const char* _name) const;
  string as_string() const;

  // # of elements for objects and arrays only
  rx_size size() const;

  template<typename F>
  void each(F&& _function) const;

private:
  json(struct json_value_s* _head);

  memory::allocator* m_allocator;
  struct json_value_s* m_root;
  struct json_parse_result_s m_error;
};

inline json::json(const string& _contents)
  : json{&memory::g_system_allocator, _contents}
{
}

inline json::operator bool() const {
  return m_root;
}

inline bool json::is_array() const {
  return m_root->type == json_type_array;
}

inline bool json::is_boolean() const {
  return m_root->type == json_type_true || m_root->type == json_type_false;
}

inline bool json::is_null() const {
  return !m_root || m_root->type == json_type_null;
}

inline bool json::is_number() const {
  return m_root->type == json_type_number;
}

inline bool json::is_object() const {
  return m_root->type == json_type_object;
}

inline bool json::is_string() const {
  return m_root->type == json_type_string;
}

template<typename F>
inline void json::each(F&& _function) const {
  for (rx_size i{0}; i < size(); i++) {
    if constexpr(traits::is_same<traits::return_type<F>, bool>) {
      if (!_function(operator[](i))) {
        return;
      }
    } else {
      _function(operator[](i));
    }
  }
}

} // namespace rx

#endif // RX_CORE_JSON_H