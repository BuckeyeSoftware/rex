#ifndef RX_CONSOLE_PARSER_H
#define RX_CONSOLE_PARSER_H

#include "rx/math/vec2.h"
#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

#include "rx/core/assert.h"
#include "rx/core/string.h"

#include "rx/core/hints/empty_bases.h"

#include "rx/core/utility/construct.h"
#include "rx/core/utility/destruct.h"

namespace rx::console {

struct RX_HINT_EMPTY_BASES token
  : concepts::no_copy
{
  enum class type {
    k_atom,
    k_string,
    k_boolean,
    k_int,
    k_float,
    k_vec4f,
    k_vec4i,
    k_vec3f,
    k_vec3i,
    k_vec2f,
    k_vec2i,
  };

  token(type _type, string&& value_);
  token(token&& token_);
  constexpr token(bool _value);
  constexpr token(rx_s32 _value);
  constexpr token(rx_f32 _value);
  constexpr token(const math::vec4f& _value);
  constexpr token(const math::vec4i& _value);
  constexpr token(const math::vec3f& _value);
  constexpr token(const math::vec3i& _value);
  constexpr token(const math::vec2f& _value);
  constexpr token(const math::vec2i& _value);
  ~token();

  token& operator=(token&& token_);

  constexpr type kind() const;

  const string& as_atom() const &;
  const string& as_string() const &;
  bool as_boolean() const;
  rx_s32 as_int() const;
  rx_f32 as_float() const;
  const math::vec4f& as_vec4f() const &;
  const math::vec4i& as_vec4i() const &;
  const math::vec3f& as_vec3f() const &;
  const math::vec3i& as_vec3i() const &;
  const math::vec2f& as_vec2f() const &;
  const math::vec2i& as_vec2i() const &;

  string print() const;

private:
  void destroy();
  void assign(token&& token_);

  type m_type;

  union {
    string m_as_atom;
    string m_as_string;
    bool m_as_boolean;
    rx_s32 m_as_int;
    rx_f32 m_as_float;
    math::vec4f m_as_vec4f;
    math::vec4i m_as_vec4i;
    math::vec3f m_as_vec3f;
    math::vec3i m_as_vec3i;
    math::vec2f m_as_vec2f;
    math::vec2i m_as_vec2i;
  };
};

inline constexpr token::token(bool _value)
  : m_type{type::k_boolean}
  , m_as_boolean{_value}
{
}

inline constexpr token::token(rx_s32 _value)
  : m_type{type::k_int}
  , m_as_int{_value}
{
}

inline constexpr token::token(rx_f32 _value)
  : m_type{type::k_float}
  , m_as_float{_value}
{
}

inline constexpr token::token(const math::vec4f& _value)
  : m_type{type::k_vec4f}
  , m_as_vec4f{_value}
{
}

inline constexpr token::token(const math::vec4i& _value)
  : m_type{type::k_vec4i}
  , m_as_vec4i{_value}
{
}

inline constexpr token::token(const math::vec3f& _value)
  : m_type{type::k_vec3f}
  , m_as_vec3f{_value}
{
}

inline constexpr token::token(const math::vec3i& _value)
  : m_type{type::k_vec3i}
  , m_as_vec3i{_value}
{
}

inline constexpr token::token(const math::vec2f& _value)
  : m_type{type::k_vec2f}
  , m_as_vec2f{_value}
{
}

inline constexpr token::token(const math::vec2i& _value)
  : m_type{type::k_vec2i}
  , m_as_vec2i{_value}
{
}

inline constexpr token::type token::kind() const {
  return m_type;
}

inline const string& token::as_atom() const & {
  RX_ASSERT(m_type == type::k_atom, "invalid type");
  return m_as_atom;
}

inline const string& token::as_string() const & {
  RX_ASSERT(m_type == type::k_string, "invalid type");
  return m_as_string;
}

inline bool token::as_boolean() const {
  RX_ASSERT(m_type == type::k_boolean, "invalid type");
  return m_as_boolean;
}

inline rx_s32 token::as_int() const {
  RX_ASSERT(m_type == type::k_int, "invalid type");
  return m_as_int;
}

inline rx_f32 token::as_float() const {
  RX_ASSERT(m_type == type::k_float, "invalid type");
  return m_as_float;
}

inline const math::vec4f& token::as_vec4f() const & {
  RX_ASSERT(m_type == type::k_vec4f, "invalid type");
  return m_as_vec4f;
}

inline const math::vec4i& token::as_vec4i() const & {
  RX_ASSERT(m_type == type::k_vec4i, "invalid type");
  return m_as_vec4i;
}

inline const math::vec3f& token::as_vec3f() const & {
  RX_ASSERT(m_type == type::k_vec3f, "invalid type");
  return m_as_vec3f;
}

inline const math::vec3i& token::as_vec3i() const & {
  RX_ASSERT(m_type == type::k_vec3i, "invalid type");
  return m_as_vec3i;
}

inline const math::vec2f& token::as_vec2f() const & {
  RX_ASSERT(m_type == type::k_vec2f, "invalid type");
  return m_as_vec2f;
}

inline const math::vec2i& token::as_vec2i() const & {
  RX_ASSERT(m_type == type::k_vec2i, "invalid type");
  return m_as_vec2i;
}

struct RX_HINT_EMPTY_BASES parser
  : concepts::no_copy
  , concepts::no_move
{
  struct diagnostic {
    diagnostic(memory::allocator& _allocator);
    string message;
    rx_size offset;
    rx_size length;
    bool inside = false;
    bool caret;
  };

  parser(memory::allocator& _allocator);

  bool parse(const string& _contents);

  const diagnostic& error() const &;
  vector<token>&& tokens();

  constexpr memory::allocator& allocator() const;

private:
  bool parse_int(const char*& contents_, rx_s32& value_);
  bool parse_float(const char*& contents_, rx_f32& value_);
  void consume_spaces();
  void record_span();

  template<typename... Ts>
  bool error(bool _caret, const char* _format, Ts&&... _arguments);

  memory::allocator& m_allocator;
  vector<token> m_tokens;
  diagnostic m_diagnostic;

  const char* m_ch;
  const char* m_first;
};

inline parser::diagnostic::diagnostic(memory::allocator& _allocator)
  : message{_allocator}
  , offset{0}
  , length{0}
  , caret{false}
{
}

inline const parser::diagnostic& parser::error() const & {
  return m_diagnostic;
}

inline vector<token>&& parser::tokens() {
  return utility::move(m_tokens);
}

RX_HINT_FORCE_INLINE constexpr memory::allocator& parser::allocator() const {
  return m_allocator;
}

template<typename... Ts>
inline bool parser::error(bool _caret, const char* _format, Ts&&... _arguments) {
  record_span();
  m_diagnostic.caret = _caret;
  if constexpr(sizeof...(Ts) != 0) {
    m_diagnostic.message =
      string::format(allocator(), _format, utility::forward<Ts>(_arguments)...);
  } else {
    m_diagnostic.message = _format;
  }
  return false;
}

const char* token_type_as_string(token::type _type);

} // namespace rx::console

#endif // RX_CONSOLE_PARSER_H
