#ifndef RX_CONSOLE_COMMAND_H
#define RX_CONSOLE_COMMAND_H
#include "rx/console/variable.h"

namespace rx::console {

// The signature specification works like this
//  b -> boolean
//  s -> string
//  i -> int
//  f -> float
//  v -> vector, followed by 2, 3, 4 for # of components, followed by i or f.
struct command
  : concepts::no_copy
{
  struct argument
    : concepts::no_copy
  {
    explicit argument(){}
    explicit argument(bool _value);
    explicit argument(const string& _value);
    explicit argument(rx_s32 _value);
    explicit argument(rx_f32 _value);
    explicit argument(const math::vec4f& _value);
    explicit argument(const math::vec4i& _value);
    explicit argument(const math::vec3f& _value);
    explicit argument(const math::vec3i& _value);
    explicit argument(const math::vec2f& _value);
    explicit argument(const math::vec2i& _value);

    argument(argument&& _argument);

    ~argument();

    variable_type type;
    union {
      bool as_boolean;
      string as_string;
      rx_s32 as_int;
      rx_f32 as_float;
      math::vec4f as_vec4f;
      math::vec4i as_vec4i;
      math::vec3f as_vec3f;
      math::vec3i as_vec3i;
      math::vec2f as_vec2f;
      math::vec2i as_vec2i;
    };
  };

  using delegate = function<bool(const vector<argument>& _arguments)>;

  command(memory::allocator* _allocator, const char* _signature, delegate&& _function);
  command(const char* _signature, delegate&& _function);
  command(command&& command_);
  ~command();

  command& operator=(command&& command_);

  template<typename... Ts>
  bool execute_arguments(Ts&&... _arguments);

  bool execute_string(const string& _contents);

private:
  bool execute();
  bool parse_string(const string& _contents);

  delegate m_delegate;
  vector<argument> m_arguments;
  const char* m_signature;
};

inline command::argument::argument(bool _value)
  : type{variable_type::k_boolean}
{
  as_boolean = _value;
}

inline command::argument::argument(const string& _value)
  : type{variable_type::k_string}
{
  utility::construct<string>(&as_string, _value);
}

inline command::argument::argument(rx_s32 _value)
  : type{variable_type::k_int}
{
  as_int = _value;
}

inline command::argument::argument(rx_f32 _value)
  : type{variable_type::k_float}
{
  as_float = _value;
}

inline command::argument::argument(const math::vec4f& _value)
  : type{variable_type::k_vec4f}
{
  as_vec4f = _value;
}

inline command::argument::argument(const math::vec4i& _value)
  : type{variable_type::k_vec4i}
{
  as_vec4i = _value;
}

inline command::argument::argument(const math::vec3f& _value)
  : type{variable_type::k_vec3f}
{
  as_vec3f = _value;
}

inline command::argument::argument(const math::vec3i& _value)
  : type{variable_type::k_vec3i}
{
  as_vec3i = _value;
}

inline command::argument::argument(const math::vec2f& _value)
  : type{variable_type::k_vec2f}
{
  as_vec2f = _value;
}

inline command::argument::argument(const math::vec2i& _value)
  : type{variable_type::k_vec2i}
{
  as_vec2i = _value;
}

inline command::command(const char* _signature, delegate&& _function)
  : command{&memory::g_system_allocator, _signature, utility::move(_function)}
{
}

inline command::command(command&& command_)
  : m_delegate{utility::move(command_.m_delegate)}
  , m_arguments{utility::move(command_.m_arguments)}
  , m_signature{command_.m_signature}
{
  command_.m_signature = nullptr;
}

inline command& command::operator=(command&& command_) {
  RX_ASSERT(&command_ != this, "self assignment");

  m_delegate = utility::move(command_.m_delegate);
  m_arguments = utility::move(command_.m_arguments);
  m_signature = command_.m_signature;

  command_.m_signature = nullptr;

  return *this;
}

template<typename... Ts>
inline bool command::execute_arguments(Ts&&... _arguments) {
  m_arguments.clear();
  (m_arguments.emplace_back(_arguments), ...);
  return execute();
}

} // namespace rx::console

#endif // RX_CONSOLE_COMMAND
