#ifndef RX_CONSOLE_VARIABLE_H
#define RX_CONSOLE_VARIABLE_H

#include <rx/core/assert.h> // RX_ASSERT
#include <rx/core/string.h> // string
#include <rx/core/statics.h> // static_global

#include <rx/math/vec2.h> // vec2{f,i}
#include <rx/math/vec3.h> // vec3{f,i}
#include <rx/math/vec4.h> // vec4{f,i}

namespace rx::console {

using ::rx::math::vec2;
using ::rx::math::vec2f;
using ::rx::math::vec2i;

using ::rx::math::vec3;
using ::rx::math::vec3f;
using ::rx::math::vec3i;

using ::rx::math::vec4;
using ::rx::math::vec4f;
using ::rx::math::vec4i;

template<typename T>
struct variable;

enum class variable_type {
  k_boolean,
  k_string,
  k_int,
  k_float,
  k_vec4f,
  k_vec4i,
  k_vec3f,
  k_vec3i,
  k_vec2f,
  k_vec2i,
};

enum class variable_status {
  k_success,
  k_out_of_range,
  k_type_mismatch,
  k_not_found,
  k_malformed
};

// type traits
template<variable_type T>
struct variable_type_trait {
  static constexpr const variable_type type{T};
};

template<typename T>
struct variable_trait;

template<> struct variable_trait<bool>   : variable_type_trait<variable_type::k_boolean> {};
template<> struct variable_trait<string> : variable_type_trait<variable_type::k_string> {};
template<> struct variable_trait<rx_s32> : variable_type_trait<variable_type::k_int> {};
template<> struct variable_trait<rx_f32> : variable_type_trait<variable_type::k_float> {};
template<> struct variable_trait<vec2f>  : variable_type_trait<variable_type::k_vec2f> {};
template<> struct variable_trait<vec2i>  : variable_type_trait<variable_type::k_vec2i> {};
template<> struct variable_trait<vec3f>  : variable_type_trait<variable_type::k_vec3f> {};
template<> struct variable_trait<vec3i>  : variable_type_trait<variable_type::k_vec3i> {};
template<> struct variable_trait<vec4f>  : variable_type_trait<variable_type::k_vec4f> {};
template<> struct variable_trait<vec4i>  : variable_type_trait<variable_type::k_vec4i> {};

static constexpr const rx_s32 k_int_min{-INT_MAX - 1};
static constexpr const rx_s32 k_int_max{INT_MAX};
static constexpr const rx_f32 k_float_min{-FLT_MAX};
static constexpr const rx_f32 k_float_max{FLT_MAX};

struct variable_reference {
  variable_reference() = default;
  variable_reference(const char* name, const char* description, void* handle, variable_type type);

  template<typename T>
  const variable<T>* try_cast() const;
  template<typename T>
  variable<T>* try_cast();

  template<typename T>
  const variable<T>* cast() const;
  template<typename T>
  variable<T>* cast();

  const char* description() const;
  const char* name() const;
  variable_type type() const;

private:
  friend struct console;

  const char* m_name;
  const char* m_description;
  void* m_handle;
  variable_type m_type;
  variable_reference* m_next;
};

// variable_referece
template<typename T>
inline const variable<T>* variable_reference::try_cast() const {
  return m_type == variable_trait<T>::type ? cast<T>() : nullptr;
}

template<typename T>
inline variable<T>* variable_reference::try_cast() {
  return m_type == variable_trait<T>::type ? cast<T>() : nullptr;
}

template<typename T>
inline const variable<T>* variable_reference::cast() const {
  RX_ASSERT(m_type == variable_trait<T>::type, "invalid cast");
  return reinterpret_cast<const variable<T>*>(m_handle);
}

template<typename T>
inline variable<T>* variable_reference::cast() {
  RX_ASSERT(m_type == variable_trait<T>::type, "invalid cast");
  return reinterpret_cast<variable<T>*>(m_handle);
}

inline const char* variable_reference::description() const {
  return m_description;
}

inline const char* variable_reference::name() const {
  return m_name;
}

inline variable_type variable_reference::type() const {
  return m_type;
}

template<typename T>
struct variable {
  variable(const char* name, const char* description, const T& min,
    const T& max, const T& initial);

  operator const T&() const;
  const T& get() const;
  const T& min() const;
  const T& max() const;
  const T& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const T& value);

private:
  variable_reference m_reference;
  T m_min;
  T m_max;
  T m_initial;
  T m_current;
};

// specialization for boolean
template<>
struct variable<bool> {
  variable(const char* name, const char* description, bool initial);

  operator const bool&() const;
  const bool& get() const;
  const bool& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(bool value);

  void toggle();

private:
  variable_reference m_reference;
  bool m_initial;
  bool m_current;
};

// specialization for string
template<>
struct variable<string> {
  variable(const char* name, const char* description, const char* initial);

  operator const string&() const;
  const string& get() const;
  const char* initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const char* value);
  variable_status set(const string& value);

private:
  variable_reference m_reference;
  const char* m_initial;
  string m_current;
};

// specialization for vector types
template<typename T>
struct variable<vec2<T>> {
  variable(const char* name, const char* description, const vec2<T>& min,
    const vec2<T>& max, const vec2<T>& initial);

  operator const vec2<T>&() const;
  const vec2<T>& get() const;
  const vec2<T>& min() const;
  const vec2<T>& max() const;
  const vec2<T>& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const vec2<T>& value);

private:
  variable_reference m_reference;
  vec2<T> m_min;
  vec2<T> m_max;
  vec2<T> m_initial;
  vec2<T> m_current;
};

template<typename T>
struct variable<vec3<T>> {
  variable(const char* name, const char* description, const vec3<T>& min,
    const vec3<T>& max, const vec3<T>& initial);

  operator const vec3<T>&() const;
  const vec3<T>& get() const;
  const vec3<T>& min() const;
  const vec3<T>& max() const;
  const vec3<T>& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const vec3<T>& value);

private:
  variable_reference m_reference;
  vec3<T> m_min;
  vec3<T> m_max;
  vec3<T> m_initial;
  vec3<T> m_current;
};

template<typename T>
struct variable<vec4<T>> {
  variable(const char* name, const char* description, const vec4<T>& min,
    const vec4<T>& max, const vec4<T>& initial);

  operator const vec4<T>&() const;
  const vec4<T>& get() const;
  const vec4<T>& min() const;
  const vec4<T>& max() const;
  const vec4<T>& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const vec4<T>& value);

private:
  variable_reference m_reference;
  vec4<T> m_min;
  vec4<T> m_max;
  vec4<T> m_initial;
  vec4<T> m_current;
};

// variable<T>
template<typename T>
inline variable<T>::variable(const char* name, const char* description,
  const T& min, const T& max, const T& initial)
  : m_reference{name, description, static_cast<void*>(this), variable_trait<T>::type}
  , m_min{min}
  , m_max{max}
  , m_initial{initial}
  , m_current{initial}
{
  // {empty}
}

template<typename T>
inline variable<T>::operator const T&() const {
  return m_current;
}

template<typename T>
inline const T& variable<T>::get() const {
  return m_current;
}

template<typename T>
inline const T& variable<T>::min() const {
  return m_min;
}

template<typename T>
inline const T& variable<T>::max() const {
  return m_max;
}

template<typename T>
inline const T& variable<T>::initial() const {
  return m_initial;
}

template<typename T>
inline const variable_reference* variable<T>::reference() const {
  return &m_reference;
}

template<typename T>
inline variable_reference* variable<T>::reference() {
  return &m_reference;
}

template<typename T>
inline void variable<T>::reset() {
  m_current = m_initial;
}

template<typename T>
inline variable_status variable<T>::set(const T& value) {
  if (value < m_min || value > m_max) {
    return variable_status::k_out_of_range;
  }
  m_current = value;
  return variable_status::k_success;
}

// variable<bool>
inline variable<bool>::variable(const char* name, const char* description, bool initial)
  : m_reference{name, description, static_cast<void*>(this), variable_trait<bool>::type}
  , m_initial{initial}
  , m_current{initial}
{
  // {empty}
}

inline variable<bool>::operator const bool&() const {
  return m_current;
}

inline const bool& variable<bool>::get() const {
  return m_current;
}

inline const bool& variable<bool>::initial() const {
  return m_initial;
}

inline variable_reference* variable<bool>::reference() {
  return &m_reference;
}

inline const variable_reference* variable<bool>::reference() const {
  return &m_reference;
}

inline void variable<bool>::reset() {
  m_current = m_initial;
}

inline variable_status variable<bool>::set(bool value) {
  m_current = value;
  return variable_status::k_success;
}

inline void variable<bool>::toggle() {
  m_current = !m_current;
}

// variable<string>
inline variable<string>::variable(const char* name, const char* description, const char* initial)
  : m_reference{name, description, static_cast<void*>(this), variable_trait<string>::type}
  , m_initial{initial}
  , m_current{initial}
{
  // {empty}
}

inline variable<string>::operator const string&() const {
  return m_current;
}

inline const string& variable<string>::get() const {
  return m_current;
}

inline const char* variable<string>::initial() const {
  return m_initial;
}

inline variable_reference* variable<string>::reference() {
  return &m_reference;
}

inline const variable_reference* variable<string>::reference() const {
  return &m_reference;
}

inline void variable<string>::reset() {
  m_current = m_initial;
}

inline variable_status variable<string>::set(const char* value) {
  m_current = value;
  return variable_status::k_success;
}

inline variable_status variable<string>::set(const string& value) {
  m_current = value;
  return variable_status::k_success;
}

// variable<vec2<T>>
template<typename T>
inline variable<vec2<T>>::variable(const char* name, const char* description,
  const vec2<T>& min, const vec2<T>& max, const vec2<T>& initial)
  : m_reference{name, description, static_cast<void*>(this), variable_trait<vec2<T>>::type}
  , m_min{min}
  , m_max{max}
  , m_initial{initial}
  , m_current{initial}
{
  // {empty}
}

template<typename T>
inline variable<vec2<T>>::operator const vec2<T>&() const {
  return m_current;
}

template<typename T>
inline const vec2<T>& variable<vec2<T>>::get() const {
  return m_current;
}

template<typename T>
inline const vec2<T>& variable<vec2<T>>::min() const {
  return m_min;
}

template<typename T>
inline const vec2<T>& variable<vec2<T>>::max() const {
  return m_max;
}

template<typename T>
inline const vec2<T>& variable<vec2<T>>::initial() const {
  return m_initial;
}

template<typename T>
inline variable_reference* variable<vec2<T>>::reference() {
  return &m_reference;
}

template<typename T>
inline const variable_reference* variable<vec2<T>>::reference() const {
  return &m_reference;
}

template<typename T>
inline void variable<vec2<T>>::reset() {
  m_current = m_initial;
}

template<typename T>
inline variable_status variable<vec2<T>>::set(const vec2<T>& value) {
  if (value.x < m_min.x || value.y < m_min.y ||
      value.x > m_max.x || value.y > m_max.y)
  {
    return variable_status::k_out_of_range;
  }
  m_current = value;
  return variable_status::k_success;
}

// variable<vec3<T>>
template<typename T>
inline variable<vec3<T>>::variable(const char* name, const char* description,
  const vec3<T>& min, const vec3<T>& max, const vec3<T>& initial)
  : m_reference{name, description, static_cast<void*>(this), variable_trait<vec3<T>>::type}
  , m_min{min}
  , m_max{max}
  , m_initial{initial}
  , m_current{initial}
{
  // {empty}
}

template<typename T>
inline variable<vec3<T>>::operator const vec3<T>&() const {
  return m_current;
}

template<typename T>
inline const vec3<T>& variable<vec3<T>>::get() const {
  return m_current;
}

template<typename T>
inline const vec3<T>& variable<vec3<T>>::min() const {
  return m_min;
}

template<typename T>
inline const vec3<T>& variable<vec3<T>>::max() const {
  return m_max;
}

template<typename T>
inline const vec3<T>& variable<vec3<T>>::initial() const {
  return m_initial;
}

template<typename T>
inline variable_reference* variable<vec3<T>>::reference() {
  return &m_reference;
}

template<typename T>
inline const variable_reference* variable<vec3<T>>::reference() const {
  return &m_reference;
}

template<typename T>
inline void variable<vec3<T>>::reset() {
  m_current = m_initial;
}

template<typename T>
inline variable_status variable<vec3<T>>::set(const vec3<T>& value) {
  if (value.x < m_min.x || value.y < m_min.y || value.z < m_min.z ||
      value.x > m_max.x || value.y > m_max.y || value.z > m_max.z)
  {
    return variable_status::k_out_of_range;
  }
  m_current = value;
  return variable_status::k_success;
}

// variable<vec4<T>>
template<typename T>
inline variable<vec4<T>>::variable(const char* name, const char* description,
  const vec4<T>& min, const vec4<T>& max, const vec4<T>& initial)
  : m_reference{name, description, static_cast<void*>(this), variable_trait<vec4<T>>::type}
  , m_min{min}
  , m_max{max}
  , m_initial{initial}
  , m_current{initial}
{
  // {empty}
}

template<typename T>
inline variable<vec4<T>>::operator const vec4<T>&() const {
  return m_current;
}

template<typename T>
inline const vec4<T>& variable<vec4<T>>::get() const {
  return m_current;
}

template<typename T>
inline const vec4<T>& variable<vec4<T>>::min() const {
  return m_min;
}

template<typename T>
inline const vec4<T>& variable<vec4<T>>::max() const {
  return m_max;
}

template<typename T>
inline const vec4<T>& variable<vec4<T>>::initial() const {
  return m_initial;
}

template<typename T>
inline variable_reference* variable<vec4<T>>::reference() {
  return &m_reference;
}

template<typename T>
inline const variable_reference* variable<vec4<T>>::reference() const {
  return &m_reference;
}

template<typename T>
inline void variable<vec4<T>>::reset() {
  m_current = m_initial;
}

template<typename T>
inline variable_status variable<vec4<T>>::set(const vec4<T>& value) {
  if (value.x < m_min.x || value.y < m_min.y || value.z < m_min.z || value.w < m_min.w ||
      value.x > m_max.x || value.y > m_max.y || value.z > m_max.z || value.w > m_max.w)
  {
    return variable_status::k_out_of_range;
  }
  m_current = value;
  return variable_status::k_success;
}

} // namespace rx::console

#define RX_CONSOLE_TRVAR(type, label, name, description, min, max, initial) \
  static ::rx::static_global<::rx::console::variable<type>> label \
    ("cvar_" name, (name), (description), (min), (max), (initial))

#define RX_CONSOLE_TUVAR(type, label, name, description, initial) \
  static ::rx::static_global<::rx::console::variable<type>> label \
    ("cvar_" name, (name), (description), (initial))

// helper macros to define console variables
#define RX_CONSOLE_BVAR(label, name, description, initial) \
  RX_CONSOLE_TUVAR(bool, label, name, (description), (initial))

#define RX_CONSOLE_SVAR(label, name, description, initial) \
  RX_CONSOLE_TUVAR(::rx::string, label, name, (description), (initial))

#define RX_CONSOLE_IVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(rx_s32, label, name, (description), (min), (max), (initial))

#define RX_CONSOLE_FVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(rx_f32, label, name, (description), (min), (max), (initial))

#define RX_CONSOLE_V2IVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec2i, label, name, (description), (min), (max), (initial))

#define RX_CONSOLE_V2FVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec2f, label, name, (description), (min), (max), (initial))

#define RX_CONSOLE_V3IVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec3i, label, name, (description), (min), (max), (initial))

#define RX_CONSOLE_V3FVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec3f, label, name, (description), (min), (max), (initial))

#define RX_CONSOLE_V4IVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec4i, label, name, (description), (min), (max), (initial))

#define RX_CONSOLE_V4FVAR(label, name, description, min, max, initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec4f, label, name, (description), (min), (max), (initial))

#endif // RX_CONSOLE_VARIABLE_H
