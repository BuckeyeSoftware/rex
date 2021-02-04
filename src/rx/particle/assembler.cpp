#include <string.h> // strcpn
#include <stdlib.h> // atoi

#include "rx/particle/assembler.h"
#include "rx/particle/vm.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Particle {

using Sink = VM::Instruction::Sink;
using Width = VM::Instruction::Width;
using OpCode = VM::Instruction::OpCode;

static constexpr const struct {
  const char* name;
  OpCode      opcode;
  Sint32      operands;
} MNEMONICS[] = {
  { "mov",  OpCode::MOV,  2 },
  { "add",  OpCode::ADD,  3 },
  { "sub",  OpCode::SUB,  3 },
  { "mul",  OpCode::MUL,  3 },
  { "mix",  OpCode::MIX,  3 },
  { "rnd",  OpCode::RND,  1 },
  { "sin",  OpCode::SIN,  2 },
  { "cos",  OpCode::COS,  2 },
  { "tan",  OpCode::TAN,  2 },
  { "asin", OpCode::ASIN, 2 },
  { "acos", OpCode::ACOS, 2 },
  { "atan", OpCode::ATAN, 2 }
};

static inline bool is_ident(int _ch) {
  return _ch >= 'a' && _ch <= 'z';
}

static inline bool is_digit(int _ch) {
  return _ch >= '0' && _ch <= '9';
}

struct Source {
  const char* name;
  const char* contents;
};

struct Lexer {
  struct Location {
    Size line   = 1;
    Size column = 1;
    Size offset = 0;
  };

  constexpr Lexer(const Source& _source)
    : m_source{_source}
  {
  }

  struct Buffer {
    char data[64];
    Size size = 0;
    void put(int _ch) {
      data[size++] = _ch;
    }
  };

  struct Token {
    enum class Type {
      IDENTIFIER,
      MNEMONIC,
      REGISTER,
      PARAMTER,
      CHANNEL,
      ERROR,
      COMMA,
      EOS
    };

    static constexpr Type sink_to_type(Sink _sink) {
      switch (_sink) {
      case Sink::CHANNEL:
        return Type::CHANNEL;
      case Sink::PARAMETER:
        return Type::PARAMTER;
      case Sink::REGISTER:
        return Type::REGISTER;
      }
      RX_HINT_UNREACHABLE();
    }

    constexpr Token() : Token{Type::EOS} {}
    constexpr Token(Type _type) : type{_type}, as_nat{} {}
    constexpr Token(Type _type, Buffer _buffer) : type{_type}, as_error{_buffer} {}
    constexpr Token(Sink _sink, Width _width, Sint32 _id) : type{sink_to_type(_sink)}, as_sink{_width, _id} {}

    Type type;

    union {
      struct {} as_nat;
      Buffer as_error;
      Buffer as_mnemonic;
      Buffer as_identifier;
      struct {
        Width width;
        Sint32 id;
      } as_sink;
    };
  };

  static constexpr const char* sink_as_str(Sink _sink) {
    switch (_sink) {
    case Sink::CHANNEL:
      return "channel";
    case Sink::PARAMETER:
      return "parameter";
    case Sink::REGISTER:
      return "register";
    }
    RX_HINT_UNREACHABLE();
  }

  Token read() {
    switch (int ch = read_ch()) {
    case '\0':
      return {Token::Type::EOS};
    case '\r': [[fallthrough]];
    case '\n':
      next_ch(); // Skip '\n', or '\r'.
      return read();
    case '#':
      // Skip until '\n'.
      for (int ch = next_ch(); ch && ch != '\n'; ch = next_ch());
      return read();
    case ' ':
      next_ch(); // Skip ' '.
      return read();
    case '%':
      return read_sink();
    case ',':
      next_ch(); // Skip ','.
      return {Token::Type::COMMA};
    case '$':
      return read_identifier();
    default:
      if (is_ident(ch)) {
        // Read mnemonic contents into buffer.
        Buffer buffer;
        while (is_ident(ch)) {
          buffer.put(ch);
          ch = next_ch();
        }
        buffer.put('\0');
        return {Token::Type::MNEMONIC, buffer};
      } else {
        return error("unexpected character '%c'", ch);
      }
    }
    RX_HINT_UNREACHABLE();
  }

  const Source& source() const { return m_source; }
  const Location& location() const { return m_location; }

private:
  Token read_sink() {
    int ch = next_ch();
    Sink sink;
    switch (ch) {
    case 'r':
      sink = Sink::REGISTER;
      break;
    case 'p':
      sink = Sink::PARAMETER;
      break;
    case 'c':
      sink = Sink::CHANNEL;
      break;
    default:
      return error("invalid sink type '%c'", ch);
    }

    ch = next_ch();
    auto width = ch == 's' ? Width::SCALAR : Width::VECTOR;
    if (ch != 's' && ch != 'v') {
      return error("invalid sink width '%c'", ch);
    }

    ch = next_ch();
    if (!is_digit(ch)) {
      return error("expected %s #", sink_as_str(sink));
    }

    int id = 0;
    while (is_digit(ch)) {
      id = id * 10 + ch - '0';
      ch = next_ch();
    }

    if (width == Width::SCALAR && id >= 32) {
      return error("invalid %s '%cs%d'", sink_as_str(sink), *sink_as_str(sink), id);
    } else if (width == Width::VECTOR && id >= 8) {
      return error("invalid %s '%cv%d'", sink_as_str(sink), *sink_as_str(sink), id);
    }

    return {sink, width, id};
  };

  Token read_identifier() {
    Buffer buffer;
    int ch = next_ch();
    if (!is_ident(ch)) {
      return error("unexpected character '%c' in identifier", ch);
    }
    while (is_ident(ch)) {
      buffer.put(ch);
      ch = next_ch();
    }
    buffer.put('\0');
    return {Token::Type::IDENTIFIER, buffer};
  }

  template<typename... Ts>
  Token error(const char* _format, Ts&&... _arguments) const {
    Token result{Token::Type::ERROR};
    result.as_error.size = format_buffer(
      result.as_error.data,
      sizeof result.as_error.data,
      _format,
      Utility::forward<Ts>(_arguments)...);
    return result;
  }

  int read_ch() const {
    return m_source.contents[m_location.offset];
  }

  int next_ch() {
    if (int ch = read_ch()) {
      m_location.offset++;
      if (ch == '\n') {
        m_location.column = 1;
        m_location.line++;
      } else {
        m_location.column++;
      }
      return read_ch();
    }
    return '\0';
  }

  Source m_source;
  Location m_location;
};

struct Parser {
  using Instruction = VM::Instruction;
  using Token = Lexer::Token;

  constexpr Parser(const Source& _source)
    : m_lexer{_source}
  {
  }

  bool parse() {
    while (next()) {
      if (m_token.type == Token::Type::EOS) {
        return true;
      }
      if (m_token.type != Token::Type::MNEMONIC) {
        return error("expected instruction mnemonic");
      }
      if (!parse_instruction()) {
        return false;
      }
    }

    return true;
  }

private:
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments) {
    const auto& source = m_lexer.source();
    const auto& location = m_lexer.location();
    m_error = String::format("%s:%zu:%zu: ", source.name, location.line, location.column)
            + String::format(_format, Utility::forward<Ts>(_arguments)...);
    return false;
  }

  bool next() {
    m_token = m_lexer.read();
    if (m_token.type == Token::Type::ERROR) {
      return error("%s", m_token.as_error.data);
    }
    return true;
  }

  bool parse_instruction() {
    for (const auto& mnemonic : MNEMONICS) {
      if (!strcmp(mnemonic.name, m_token.as_mnemonic.data)) {
        return next() && parse_instruction(mnemonic.opcode, mnemonic.operands);
      }
    }
    return error("invalid instruction \"%s\"", m_token.as_mnemonic.data);
  }

  Optional<VM::Instruction::Operand> parse_operand() {
    Instruction::Operand result;
    switch (m_token.type) {
    case Token::Type::CHANNEL:
      result.s = Sink::CHANNEL;
      result.i = m_token.as_sink.id;
      result.w = m_token.as_sink.width;
      return result;
    case Token::Type::PARAMTER:
      result.s = Sink::PARAMETER;
      result.i = m_token.as_sink.id;
      result.w = m_token.as_sink.width;
      return result;
    case Token::Type::REGISTER:
      result.s = Sink::REGISTER;
      result.i = m_token.as_sink.id;
      result.w = m_token.as_sink.width;
      return result;
    default:
      error("expected sink operand");
    }
    return nullopt;
  }

  bool parse_instruction(OpCode _opcode, Sint32 _operands) {
    VM::Instruction instruction{};
    instruction.opcode = _opcode;
    for (Sint32 i = 0; i < _operands; i++) {
      if (i && (!next() || !expect_and_skip_comma())) {
        return false;
      }
      if (auto operand = parse_operand()) {
        instruction.ops[i] = *operand;
      } else {
        return false;
      }
    }
    return m_instructions.push_back(instruction);
  }

  bool expect_and_skip_comma() {
    if (m_token.type != Token::Type::COMMA) {
      return error("expected comma");
    }
    return next();
  }

  Lexer m_lexer;
  Token m_token;

  friend struct Assembler;

  Vector<VM::Instruction> m_instructions;
  String m_error;
};

bool Assembler::assemble(const String& _file_name) {
  auto file = Filesystem::read_text_file(_file_name);
  if (!file) {
    return false;
  }

  auto name = _file_name.data();
  auto contents = reinterpret_cast<const char*>(file->data());

  Parser parser{{name, contents}};
  if (parser.parse()) {
    m_instructions = parser.m_instructions.disown();
    return true;
  }

  m_error = Utility::move(parser.m_error);
  return false;
}

} // namespace Rx::Particle