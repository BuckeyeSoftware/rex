#ifndef RX_CONSOLE_VARIABLE_H
#define RX_CONSOLE_VARIABLE_H
#include <limits.h> // INT_{MIN, MAX}

#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/string.h" // string
#include "rx/core/global.h" // global
#include "rx/core/event.h" // event

#include "rx/math/vec2.h" // vec2{f,i}
#include "rx/math/vec3.h" // vec3{f,i}
#include "rx/math/vec4.h" // vec4{f,i}

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
  k_type_mismatch
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
  variable_reference(const char* _name, const char* _description, void* _handle, variable_type _type);

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

  void reset();
  string print_current() const;
  string print_range() const;
  string print_initial() const;

  bool is_initial() const;

private:
  friend struct interface;

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
  using on_change_event = event<void(variable<T>&)>;

  variable(const char* _name, const char* _description, const T& _min,
    const T& _max, const T& _initial);

  operator const T&() const;
  const T& get() const;
  const T& min() const;
  const T& max() const;
  const T& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const T& value);

  typename on_change_event::handle on_change(typename on_change_event::delegate&& _on_change);

private:
  variable_reference m_reference;
  T m_min;
  T m_max;
  T m_initial;
  T m_current;
  on_change_event m_on_change;
};

// specialization for boolean
template<>
struct variable<bool> {
  using on_change_event = event<void(variable<bool>&)>;

  variable(const char* _name, const char* _description, bool _initial);

  operator const bool&() const;
  const bool& get() const;
  const bool& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(bool value);

  void toggle();

  typename on_change_event::handle on_change(typename on_change_event::delegate&& _on_change);

private:
  variable_reference m_reference;
  bool m_initial;
  bool m_current;
  on_change_event m_on_change;
};

// specialization for string
template<>
struct variable<string> {
  using on_change_event = event<void(variable<string>&)>;

  variable(const char* _name, const char* _description, const char* _initial);

  operator const string&() const;
  const string& get() const;
  const char* initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const char* _value);
  variable_status set(const string& _value);

  typename on_change_event::handle on_change(typename on_change_event::delegate&& _on_change);

private:
  variable_reference m_reference;
  const char* m_initial;
  string m_current;
  on_change_event m_on_change;
};

// specialization for vector types
template<typename T>
struct variable<vec2<T>> {
  using on_change_event = event<void(variable<vec2<T>>&)>;

  variable(const char* _name, const char* _description, const vec2<T>& _min,
    const vec2<T>& _max, const vec2<T>& _initial);

  operator const vec2<T>&() const;
  const vec2<T>& get() const;
  const vec2<T>& min() const;
  const vec2<T>& max() const;
  const vec2<T>& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const vec2<T>& _value);

  typename on_change_event::handle on_change(typename on_change_event::delegate&& _on_change);

private:
  variable_reference m_reference;
  vec2<T> m_min;
  vec2<T> m_max;
  vec2<T> m_initial;
  vec2<T> m_current;
  on_change_event m_on_change;
};

template<typename T>
struct variable<vec3<T>> {
  using on_change_event = event<void(variable<vec3<T>>&)>;

  variable(const char* _name, const char* _description, const vec3<T>& _min,
    const vec3<T>& _max, const vec3<T>& _initial);

  operator const vec3<T>&() const;
  const vec3<T>& get() const;
  const vec3<T>& min() const;
  const vec3<T>& max() const;
  const vec3<T>& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const vec3<T>& value);

  typename on_change_event::handle on_change(typename on_change_event::delegate&& _on_change);

private:
  variable_reference m_reference;
  vec3<T> m_min;
  vec3<T> m_max;
  vec3<T> m_initial;
  vec3<T> m_current;
  on_change_event m_on_change;
};

template<typename T>
struct variable<vec4<T>> {
  using on_change_event = event<void(variable<vec4<T>>&)>;

  variable(const char* _name, const char* _description, const vec4<T>& _min,
    const vec4<T>& _max, const vec4<T>& _initial);

  operator const vec4<T>&() const;
  const vec4<T>& get() const;
  const vec4<T>& min() const;
  const vec4<T>& max() const;
  const vec4<T>& initial() const;

  variable_reference* reference();
  const variable_reference* reference() const;

  void reset();
  variable_status set(const vec4<T>& value);

  typename on_change_event::handle on_change(typename on_change_event::delegate&& _on_change);

private:
  variable_reference m_reference;
  vec4<T> m_min;
  vec4<T> m_max;
  vec4<T> m_initial;
  vec4<T> m_current;
  on_change_event m_on_change;
};

// variable<T>
template<typename T>
inline variable<T>::variable(const char* _name, const char* _description,
  const T& _min, const T& _max, const T& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), variable_trait<T>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
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
inline variable_status variable<T>::set(const T& _value) {
  if (_value < m_min || _value > m_max) {
    return variable_status::k_out_of_range;
  }

  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }

  return variable_status::k_success;
}

template<typename T>
inline typename variable<T>::on_change_event::handle variable<T>::on_change(typename on_change_event::delegate&& on_change_) {
  return m_on_change.connect(utility::move(on_change_));
}

// variable<bool>
inline variable<bool>::variable(const char* _name, const char* _description, bool _initial)
  : m_reference{_name, _description, static_cast<void*>(this), variable_trait<bool>::type}
  , m_initial{_initial}
  , m_current{_initial}
{
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
  if (m_current != value) {
    m_current = value;
    m_on_change.signal(*this);
  }
  return variable_status::k_success;
}

inline void variable<bool>::toggle() {
  m_current = !m_current;
  m_on_change.signal(*this);
}

inline typename variable<bool>::on_change_event::handle variable<bool>::on_change(typename on_change_event::delegate&& on_change_) {
  return m_on_change.connect(utility::move(on_change_));
}

// variable<string>
inline variable<string>::variable(const char* _name, const char* _description, const char* _initial)
  : m_reference{_name, _description, static_cast<void*>(this), variable_trait<string>::type}
  , m_initial{_initial}
  , m_current{_initial}
{
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

inline variable_status variable<string>::set(const char* _value) {
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }
  return variable_status::k_success;
}

inline variable_status variable<string>::set(const string& _value) {
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }

  return variable_status::k_success;
}

inline typename variable<string>::on_change_event::handle variable<string>::on_change(typename on_change_event::delegate&& on_change_) {
  return m_on_change.connect(utility::move(on_change_));
}

// variable<vec2<T>>
template<typename T>
inline variable<vec2<T>>::variable(const char* _name, const char* _description,
  const vec2<T>& _min, const vec2<T>& _max, const vec2<T>& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), variable_trait<vec2<T>>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
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
inline variable_status variable<vec2<T>>::set(const vec2<T>& _value) {
  if (_value.x < m_min.x || _value.y < m_min.y ||
      _value.x > m_max.x || _value.y > m_max.y)
  {
    return variable_status::k_out_of_range;
  }

  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }

  return variable_status::k_success;
}

template<typename T>
inline typename variable<vec2<T>>::on_change_event::handle variable<vec2<T>>::on_change(typename on_change_event::delegate&& on_change_) {
  return m_on_change.connect(utility::move(on_change_));
}

// variable<vec3<T>>
template<typename T>
inline variable<vec3<T>>::variable(const char* _name, const char* _description,
  const vec3<T>& _min, const vec3<T>& _max, const vec3<T>& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), variable_trait<vec3<T>>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
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
inline variable_status variable<vec3<T>>::set(const vec3<T>& _value) {
  if (_value.x < m_min.x || _value.y < m_min.y || _value.z < m_min.z ||
      _value.x > m_max.x || _value.y > m_max.y || _value.z > m_max.z)
  {
    return variable_status::k_out_of_range;
  }
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }
  return variable_status::k_success;
}

template<typename T>
inline typename variable<vec3<T>>::on_change_event::handle variable<vec3<T>>::on_change(typename on_change_event::delegate&& on_change_) {
  return m_on_change.connect(utility::move(on_change_));
}

// variable<vec4<T>>
template<typename T>
inline variable<vec4<T>>::variable(const char* _name, const char* _description,
  const vec4<T>& _min, const vec4<T>& _max, const vec4<T>& _initial)
  : m_reference{_name, _description, static_cast<void*>(this), variable_trait<vec4<T>>::type}
  , m_min{_min}
  , m_max{_max}
  , m_initial{_initial}
  , m_current{_initial}
{
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
inline variable_status variable<vec4<T>>::set(const vec4<T>& _value) {
  if (_value.x < m_min.x || _value.y < m_min.y || _value.z < m_min.z || _value.w < m_min.w ||
      _value.x > m_max.x || _value.y > m_max.y || _value.z > m_max.z || _value.w > m_max.w)
  {
    return variable_status::k_out_of_range;
  }
  if (m_current != _value) {
    m_current = _value;
    m_on_change.signal(*this);
  }
  return variable_status::k_success;
}

template<typename T>
inline typename variable<vec4<T>>::on_change_event::handle variable<vec4<T>>::on_change(typename on_change_event::delegate&& on_change_) {
  return m_on_change.connect(utility::move(on_change_));
}

const char* variable_type_as_string(variable_type _type);

inline bool variable_type_is_ranged(variable_type _type) {
  return _type != variable_type::k_boolean && _type != variable_type::k_string;
}

} // namespace rx::console

#define RX_CONSOLE_TRVAR(_type, _label, _name, _description, _min, _max, _initial) \
  static ::rx::global<::rx::console::variable<_type>> _label \
    {"cvars", (_name), (_name), (_description), (_min), (_max), (_initial)}

#define RX_CONSOLE_TUVAR(_type, _label, _name, _description, _initial) \
  static ::rx::global<::rx::console::variable<_type>> _label \
    {"cvars", (_name), (_name), (_description), (_initial)}

// helper macros to define console variables
#define RX_CONSOLE_BVAR(_label, _name, _description, _initial) \
  RX_CONSOLE_TUVAR(bool, _label, _name, (_description), (_initial))

#define RX_CONSOLE_SVAR(_label, _name, _description, _initial) \
  RX_CONSOLE_TUVAR(::rx::string, _label, _name, (_description), (_initial))

#define RX_CONSOLE_IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(rx_s32, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(rx_f32, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V2IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec2i, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V2FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec2f, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V3IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec3i, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V3FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec3f, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V4IVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec4i, _label, _name, (_description), (_min), (_max), (_initial))

#define RX_CONSOLE_V4FVAR(_label, _name, _description, _min, _max, _initial) \
  RX_CONSOLE_TRVAR(::rx::math::vec4f, _label, _name, (_description), (_min), (_max), (_initial))

#endif // RX_CONSOLE_VARIABLE_H
