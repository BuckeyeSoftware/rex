#include "rx/particle/vm.h"
#include "rx/particle/program.h"

#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"
#include "rx/core/math/tan.h"
#include "rx/core/math/floor.h"
#include "rx/core/math/ceil.h"

#include "rx/core/random/context.h"

#include "rx/core/algorithm/clamp.h"

namespace Rx::Particle {

static inline auto mix(auto x, auto y, Float32 a) {
  return x * (1.0f - a) + y * a;
}

static inline Float32 rnd(Random::Context& _random, Float32 _min, Float32 _max) {
  return (_random.f32() * (_max - _min)) + _min;
}

VM::Result VM::execute(Random::Context& _random, const Parameters& _parameters,
  const Program& _program)
{
  Result result;

  auto rd_s = [this](Uint8 _i) RX_HINT_FORCE_INLINE_LAMBDA {
    return m_registers.s[_i];
  };

  auto rd_v = [this](Uint8 _i) RX_HINT_FORCE_INLINE_LAMBDA -> Math::Vec4f {
    return m_registers.v[_i];
  };

  auto wr_s = [this](Uint8 _i, auto _value) RX_HINT_FORCE_INLINE_LAMBDA {
    m_registers.s[_i] = _value;
  };

  auto wr_v = [this](Uint8 _i, auto _value) RX_HINT_FORCE_INLINE_LAMBDA {
    auto& vector = m_registers.v[_i];
    vector[0] = _value[0];
    vector[1] = _value[1];
    vector[2] = _value[2];
    vector[3] = _value[3];
  };

  // Load a scalar.
  auto scalar_load = [&](Instruction::Operand _operand) {
    switch (_operand.s) {
    case Instruction::Sink::CHANNEL:
      switch (_operand.i) {
      case Channel::LIFE:
        return result.life;
      case Channel::SIZE:
        return result.size;
      case Channel::TEXTURE:
        return result.texture;
      }
      break;
    case Instruction::Sink::PARAMETER:
      return _parameters[_operand.i];
    case Instruction::Sink::REGISTER:
      return rd_s(_operand.i);
    }
    return 0.0f;
  };

  auto scalar_store = [&](Instruction::Operand _operand, Float32 _value) {
    switch (_operand.s) {
    case Instruction::Sink::CHANNEL:
      result.mask |= 1 << _operand.i;
      switch (_operand.i) {
      case Channel::LIFE:
        result.life = _value;
        return;
      case Channel::SIZE:
        result.size = _value;
        return;
      case Channel::TEXTURE:
        result.texture = Math::floor(_value);
        return;
      }
      break;
    case Instruction::Sink::PARAMETER:
      // Undefined.
      return;
    case Instruction::Sink::REGISTER:
      return wr_s(_operand.i, _value);
    }
  };

  auto vector_load = [&](Instruction::Operand _operand) -> Math::Vec4f {
    switch (_operand.s) {
    case Instruction::Sink::CHANNEL:
      switch (_operand.i) {
      case Channel::ACCELERATION:
        return result.acceleration;
      case Channel::COLOR:
        return result.color;
      case Channel::POSITION:
        return result.position;
      case Channel::VELOCITY:
        return result.velocity;
      }
      break;
    case Instruction::Sink::PARAMETER:
      return {
        _parameters[_operand.i * 4 + 0],
        _parameters[_operand.i * 4 + 1],
        _parameters[_operand.i * 4 + 2],
        _parameters[_operand.i * 4 + 3]
      };
    case Instruction::Sink::REGISTER:
      return rd_v(_operand.i);
    }
    return {};
  };

  auto vector_store_v = [&](Instruction::Operand _operand, const Math::Vec4f& _value) {
    switch (_operand.s) {
    case Instruction::Sink::CHANNEL:
      result.mask |= 1 << _operand.i;
      switch (_operand.i) {
      case Channel::ACCELERATION:
        result.acceleration = _value;
        break;
      case Channel::COLOR:
        result.color = _value;
        break;
      case Channel::POSITION:
        result.position = _value;
        break;
      case Channel::VELOCITY:
        result.velocity = _value;
        break;
      }
      break;
    case Instruction::Sink::PARAMETER:
      // Undefined.
      return;
    case Instruction::Sink::REGISTER:
      wr_v(_operand.i, _value);
      break;
    }
  };

  // Similar to vector_store_v but splats |_value| on all lanes.
  auto vector_store_s = [&](Instruction::Operand _operand, Float32 _value) RX_HINT_FORCE_INLINE_LAMBDA {
    return vector_store_v(_operand, {_value, _value, _value, _value});
  };

  // SCALAR <op> VECTOR is undefined.
  auto binop = [&](const Instruction& _instruction, auto&& _f) RX_HINT_FORCE_INLINE_LAMBDA {
    switch (_instruction.a.w) {
    case Instruction::Width::SCALAR:
      return wr_s(_instruction.a.i, _f(rd_s(_instruction.b.i), rd_s(_instruction.c.i)));
    case Instruction::Width::VECTOR:
      switch (_instruction.c.w) {
      case Instruction::Width::SCALAR:
        return wr_v(_instruction.a.i, _f(rd_v(_instruction.b.i), rd_s(_instruction.c.i)));
      case Instruction::Width::VECTOR:
        return wr_v(_instruction.a.i, _f(rd_v(_instruction.b.i), rd_v(_instruction.c.i)));
      }
    }
  };

  auto read_imm_scalar = [&](Uint8 _lo, Uint8 _hi) RX_HINT_FORCE_INLINE_LAMBDA {
    const auto index = Uint16(_lo) << 8 | Uint16(_hi);
    return _program.data[index];
  };

  auto read_imm_vector = [&](Uint8 _lo, Uint8 _hi) RX_HINT_FORCE_INLINE_LAMBDA {
    const auto index = Uint16(_lo) << 8 | Uint16(_hi);
    return Math::Vec4f {
      _program.data[index + 0],
      _program.data[index + 1],
      _program.data[index + 2],
      _program.data[index + 3]
    };
  };

  // a = unary(a)
  auto unary = [&](const Instruction& _instruction, auto&& _f) RX_HINT_FORCE_INLINE_LAMBDA {
    switch (_instruction.a.w) {
    case Instruction::Width::SCALAR:
      return wr_s(_instruction.a.i, _f(rd_s(_instruction.b.i)));
    case Instruction::Width::VECTOR:
      return wr_v(_instruction.a.i, rd_v(_instruction.b.i).map(_f));
    }
  };

  // a = bin(b, c)
  auto bin = [&](const Instruction& _instruction, auto&& _f) RX_HINT_FORCE_INLINE_LAMBDA {
    switch (_instruction.a.w) {
    case Instruction::Width::SCALAR:
      return wr_s(_instruction.a.i, _f(rd_s(_instruction.b.i), rd_s(_instruction.c.i)));
    case Instruction::Width::VECTOR:
      return wr_v(_instruction.a.i,
        Math::Vec4f {
          _f(rd_v(_instruction.b.i).x, rd_v(_instruction.c.i).x),
          _f(rd_v(_instruction.b.i).y, rd_v(_instruction.c.i).y),
          _f(rd_v(_instruction.b.i).z, rd_v(_instruction.c.i).z),
          _f(rd_v(_instruction.b.i).w, rd_v(_instruction.c.i).w)
        });
    }
  };

#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
  static constexpr void* DISPATCH_TABLE[] = {
    &&LI, &&MOV, &&ADD, &&SUB, &&MUL, &&MIX, &&CLAMP, &&RND, &&SIN, &&COS, &&TAN,
    &&ASIN, &&ACOS, &&ATAN, &&SQRT, &&ABS, &&CEIL, &&FLOOR, &&FRACT, &&MIN, &&MAX,
    &&HLT
  };
#define CASE(_name) _name:
#define BREAK() DISPATCH((++instruction)->opcode)
#define SWITCH(code) DISPATCH(code)
#define DISPATCH(code) goto *DISPATCH_TABLE[Size(code)];
#else
#define CASE(_name) case Instruction::OpCode::_name:
#define BREAK() break
#define SWITCH(code) switch (code)
#define DISPATCH(code)
#endif

  const auto* instruction =
    reinterpret_cast<const Instruction*>(_program.instructions.data());

  DISPATCH(instruction->opcode);

  for (;; instruction++) {
    SWITCH(instruction->opcode) {
    CASE(HLT)
      return result;
    CASE(LI)
      switch (instruction->a.w) {
      case Instruction::Width::SCALAR:
        wr_s(instruction->a.i, read_imm_scalar(instruction->b.u8, instruction->c.u8));
        break;
      case Instruction::Width::VECTOR:
        wr_v(instruction->a.i, read_imm_vector(instruction->b.u8, instruction->c.u8));
        break;
      }
      BREAK();
    // MOV dst:SCALAR, src:SCALAR - Copies src to dst.
    // MOV dst:SCALAR, src:VECTOR - Copies src's component c.i to dst.
    // MOV dst:VECTOR, src:SCALAR - Splats src to all lanes of dst.
    // MOV dst:VECTOR, src:VECTOR - Copies src to dst.
    CASE(MOV)
      switch (instruction->a.w) {
      case Instruction::Width::SCALAR:
        switch (instruction->b.w) {
        case Instruction::Width::SCALAR:
          // Scalar load -> Scalar store.
          scalar_store(instruction->a, scalar_load(instruction->b));
          break;
        case Instruction::Width::VECTOR:
          // Vector load -> Scalar store.
          scalar_store(instruction->a, vector_load(instruction->b)[instruction->c.i]);
          break;
        }
        break;
      case Instruction::Width::VECTOR:
        switch (instruction->b.w) {
        case Instruction::Width::SCALAR:
          // Scalar load -> Vector store.
          vector_store_s(instruction->a, scalar_load(instruction->b));
          break;
        case Instruction::Width::VECTOR:
          // Vector load -> Vector store.
          vector_store_v(instruction->a, vector_load(instruction->b));
          break;
        }
        break;
      }
      BREAK();
    // All binary arithmetic operations are defined for:
    //  Scalar <op> Scalar => Scalar
    //  Vector <op> Scalar => Vector
    //  Vector <op> Vector => Vector
    CASE(ADD)
      binop(*instruction, [](auto a, auto b) { return a + b; });
      BREAK();
    CASE(SUB)
      binop(*instruction, [](auto a, auto b) { return a - b; });
      BREAK();
    CASE(MUL)
      binop(*instruction, [](auto a, auto b) { return a * b; });
      BREAK();
    // Defined for SCALAR and VECTOR (component-wise)
    CASE(MIN)
      bin(*instruction, [](auto a, auto b) { return Algorithm::min(a, b); });
      BREAK();
    CASE(MAX)
      bin(*instruction, [](auto a, auto b) { return Algorithm::max(a, b); });
      BREAK();
    CASE(MIX)
      // Only valid for a.Width == b.Width && c.Width = SCALAR.
      // Clobbers %s0 for SCALAR or %v0 for VECTOR.
      switch (instruction->a.w) {
      case Instruction::Width::SCALAR:
        wr_s(0, mix(rd_s(instruction->a.i), rd_s(instruction->b.i), rd_s(instruction->c.i)));
        break;
      case Instruction::Width::VECTOR:
        wr_v(0, mix(rd_v(instruction->a.i), rd_v(instruction->b.i), rd_s(instruction->c.i)));
        break;
      }
      BREAK();
    CASE(CLAMP)
      // Only valid for b.Width = SCALAR && c.Width = SCALAR
      switch (instruction->a.w) {
      // Scalar clamp
      case Instruction::Width::SCALAR:
        wr_s(0, Algorithm::clamp(rd_s(instruction->a.i), rd_s(instruction->b.i), rd_s(instruction->c.i)));
        break;
      // Component wise clamp.
      case Instruction::Width::VECTOR:
        wr_v(0, rd_v(instruction->a.i).map([&](auto _value) {
          return Algorithm::clamp(_value, rd_s(instruction->b.i), rd_s(instruction->c.i)); }));
      }
      BREAK();
    CASE(RND)
      switch (instruction->a.w) {
      case Instruction::Width::SCALAR:
        wr_s(instruction->a.i, rnd(_random, 0.0f, 1.0f));
        break;
      case Instruction::Width::VECTOR:
        wr_v(
          instruction->a.i,
          Math::Vec4f {
            rnd(_random, 0.0f, 1.0f),
            rnd(_random, 0.0f, 1.0f),
            rnd(_random, 0.0f, 1.0f),
            rnd(_random, 0.0f, 1.0f)});
        break;
      }
      BREAK();
    // These unary operations are defined for SCALAR and VECTOR (component-wise)
    CASE(ABS)
      unary(*instruction, [](auto _value) { return Math::abs(_value); });
      BREAK();
    CASE(CEIL)
      unary(*instruction, [](auto _value) { return Math::ceil(_value); });
      BREAK();
    CASE(FLOOR)
      unary(*instruction, [](auto _value) { return Math::floor(_value); });
      BREAK();
    CASE(FRACT)
      unary(*instruction, [](auto _value) { return Math::floor(_value); });
      BREAK();
    CASE(SIN)
      unary(*instruction, [](auto _value) { return Math::sin(_value); });
      BREAK();
    CASE(COS)
      unary(*instruction, [](auto _value) { return Math::cos(_value); });
      BREAK();
    CASE(TAN)
      unary(*instruction, [](auto _value) { return Math::tan(_value); });
      BREAK();
    CASE(ASIN)
      unary(*instruction, [](auto _value) { return Math::asin(_value); });
      BREAK();
    CASE(ACOS)
      unary(*instruction, [](auto _value) { return Math::acos(_value); });
      BREAK();
    CASE(ATAN)
      unary(*instruction, [](auto _value) { return Math::atan(_value); });
      BREAK();
    CASE(SQRT)
      unary(*instruction, [](auto _value) { return Math::sqrt(_value); });
      BREAK();
    }
  }

  RX_HINT_UNREACHABLE();
}

} // namespace Rx::Particle