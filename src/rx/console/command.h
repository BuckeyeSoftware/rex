#ifndef RX_CONSOLE_COMMAND_H
#define RX_CONSOLE_COMMAND_H
#include "rx/console/variable.h"
#include "rx/console/parser.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/core/utility/exchange.h"

namespace rx::console {

// The signature specification works like this
//  b -> boolean
//  s -> string
//  i -> int
//  f -> float
//  v -> vector, followed by 2, 3, 4 for # of components, followed by i or f.
struct RX_HINT_EMPTY_BASES command
  : concepts::no_copy
{
  struct RX_HINT_EMPTY_BASES argument
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

  command(memory::allocator& _allocator, const string& _name,
    const char* _signature, delegate&& _function);
  command(const string& _name, const char* _signature, delegate&& _function);
  command(command&& command_);

  command& operator=(command&& command_);

  template<typename... Ts>
  bool execute_arguments(Ts&&... _arguments);

  bool execute_tokens(const vector<token>& _tokens);

  const string& name() const &;

  constexpr memory::allocator& allocator() const;

private:
  bool execute();

  ref<memory::allocator> m_allocator;
  delegate m_delegate;
  vector<argument> m_arguments;
  string m_declaration;
  string m_name;
  const char* m_signature;
  rx_size m_argument_count;
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

inline command::command(const string& _name, const char* _signature, delegate&& _function)
  : command{memory::system_allocator::instance(), _name, _signature, utility::move(_function)}
{
}

inline command::command(command&& command_)
  : m_allocator{command_.m_allocator}
  , m_delegate{utility::move(command_.m_delegate)}
  , m_arguments{utility::move(command_.m_arguments)}
  , m_declaration{utility::move(command_.m_declaration)}
  , m_name{utility::move(command_.m_name)}
  , m_signature{utility::exchange(command_.m_signature, nullptr)}
  , m_argument_count{utility::exchange(command_.m_argument_count, 0)}
{
}

inline command& command::operator=(command&& command_) {
  RX_ASSERT(&command_ != this, "self assignment");

  m_allocator = command_.m_allocator;
  m_delegate = utility::move(command_.m_delegate);
  m_arguments = utility::move(command_.m_arguments);
  m_declaration = utility::move(command_.m_declaration);
  m_name = utility::move(command_.m_name);
  m_signature = utility::exchange(command_.m_signature, nullptr);
  m_argument_count = utility::exchange(command_.m_argument_count, 0);

  return *this;
}

template<typename... Ts>
inline bool command::execute_arguments(Ts&&... _arguments) {
  m_arguments.clear();
  (m_arguments.emplace_back(_arguments), ...);
  return execute();
}

inline const string& command::name() const & {
  return m_name;
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& command::allocator() const {
  return m_allocator;
}

} // namespace rx::console

#endif // RX_CONSOLE_COMMAND
