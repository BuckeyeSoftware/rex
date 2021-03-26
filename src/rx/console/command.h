#ifndef RX_CONSOLE_COMMAND_H
#define RX_CONSOLE_COMMAND_H
#include "rx/console/variable.h"
#include "rx/console/parser.h"

#include "rx/core/markers.h"

namespace Rx::Console {

struct Context;

// The signature specification works like this
//  b -> boolean
//  s -> string
//  i -> int
//  f -> float
//  v -> vector, followed by 2, 3, 4 for # of components, followed by i or f.
struct Command {
  RX_MARK_NO_COPY(Command);

  struct Argument {
    RX_MARK_NO_COPY(Argument);

    explicit Argument(){}
    explicit Argument(bool _value);
    explicit Argument(const String& _value);
    explicit Argument(Sint32 _value);
    explicit Argument(Float32 _value);
    explicit Argument(const Math::Vec4f& _value);
    explicit Argument(const Math::Vec4i& _value);
    explicit Argument(const Math::Vec3f& _value);
    explicit Argument(const Math::Vec3i& _value);
    explicit Argument(const Math::Vec2f& _value);
    explicit Argument(const Math::Vec2i& _value);

    Argument(Argument&& _argument);

    ~Argument();

    VariableType type;
    union {
      Bool as_boolean;
      String as_string;
      Sint32 as_int;
      Float32 as_float;
      Math::Vec4f as_vec4f;
      Math::Vec4i as_vec4i;
      Math::Vec3f as_vec3f;
      Math::Vec3i as_vec3i;
      Math::Vec2f as_vec2f;
      Math::Vec2i as_vec2i;
    };
  };

  using Delegate = Function<bool(Context& console_, const Vector<Argument>& _arguments)>;

  static Optional<Command> create(Memory::Allocator& _allocator,
    const String& _name, const char* _signature, Delegate&& function_);

  Command(Command&& command_);
  Command& operator=(Command&& command_);

  template<typename... Ts>
  [[nodiscard]] bool execute_arguments(Context& console_, Ts&&... _arguments);

  [[nodiscard]] bool execute_tokens(Context& console_, const Vector<Token>& _tokens);

  const String& name() const &;

private:
  [[nodiscard]] bool execute(Context& console_);

  Command(Delegate&& delegate_, Vector<Argument>&& arguments_,
    Vector<VariableType>&& signature_, String&& name_);

  Delegate m_delegate;
  Vector<Argument> m_arguments;
  Vector<VariableType> m_signature;
  String m_name;
};

inline Command::Argument::Argument(bool _value)
  : type{VariableType::BOOLEAN}
{
  as_boolean = _value;
}

inline Command::Argument::Argument(const String& _value)
  : type{VariableType::STRING}
{
  Utility::construct<String>(&as_string, _value);
}

inline Command::Argument::Argument(Sint32 _value)
  : type{VariableType::INT}
{
  as_int = _value;
}

inline Command::Argument::Argument(Float32 _value)
  : type{VariableType::FLOAT}
{
  as_float = _value;
}

inline Command::Argument::Argument(const Math::Vec4f& _value)
  : type{VariableType::VEC4F}
{
  as_vec4f = _value;
}

inline Command::Argument::Argument(const Math::Vec4i& _value)
  : type{VariableType::VEC4I}
{
  as_vec4i = _value;
}

inline Command::Argument::Argument(const Math::Vec3f& _value)
  : type{VariableType::VEC3F}
{
  as_vec3f = _value;
}

inline Command::Argument::Argument(const Math::Vec3i& _value)
  : type{VariableType::VEC3I}
{
  as_vec3i = _value;
}

inline Command::Argument::Argument(const Math::Vec2f& _value)
  : type{VariableType::VEC2F}
{
  as_vec2f = _value;
}

inline Command::Argument::Argument(const Math::Vec2i& _value)
  : type{VariableType::VEC2I}
{
  as_vec2i = _value;
}

inline Command::Command(Command&&) = default;
inline Command& Command::operator=(Command&&) = default;

template<typename... Ts>
bool Command::execute_arguments(Context& console_, Ts&&... _arguments) {
  m_arguments.clear();
  return (m_arguments.emplace_back(_arguments) && ...) && execute(console_);
}

inline const String& Command::name() const & {
  return m_name;
}

} // namespace Rx::Console

#endif // RX_CONSOLE_COMMAND
