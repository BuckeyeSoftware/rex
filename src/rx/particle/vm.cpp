#include <stdlib.h> // rand, RAND_MAX.
#include <stdio.h>
#include <string.h> // memcpy

#include "rx/particle/vm.h"
#include "rx/particle/program.h"

#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"
#include "rx/core/math/tan.h"

namespace Rx::Particle {

static inline auto mix(auto x, auto y, Float32 a) {
  return x * (1.0f - a) + y * a;
}

static inline Float32 rnd(Float32 _min, Float32 _max) {
  return ((Float32(rand()) / Float32(RAND_MAX)) * (_max - _min)) + _min;
}

VM::Result VM::execute(const Parameters& _parameters, const Program& _program) {
  Result result;

  auto rd_s = [this](Uint8 _i) {
    return m_registers.s[_i];
  };

  auto rd_v = [this](Uint8 _i) -> Math::Vec4f {
    return m_registers.v[_i];
  };

  auto wr_s = [this](Uint8 _i, auto _value) {
    m_registers.s[_i] = _value;
  };

  auto wr_v = [this](Uint8 _i, auto _value) {
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
      }
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
  auto vector_store_s = [&](Instruction::Operand _operand, Float32 _value) {
    return vector_store_v(_operand, {_value, _value, _value, _value});
  };

  // SCALAR <op> VECTOR is undefined.
  #define BINOP(_op) \
    switch (instruction.a.w) { \
    case Instruction::Width::SCALAR: \
      wr_s(instruction.a.i, rd_s(instruction.b.i) _op rd_s(instruction.c.i)); \
      break; \
    case Instruction::Width::VECTOR: \
      switch (instruction.c.w) { \
      case Instruction::Width::SCALAR: \
        wr_v(instruction.a.i, rd_v(instruction.b.i) _op rd_s(instruction.c.i)); \
        break; \
      case Instruction::Width::VECTOR: \
        wr_v(instruction.a.i, rd_v(instruction.b.i) _op rd_v(instruction.c.i)); \
        break; \
      } \
      break; \
    }

  auto read_imm_scalar = [&](Uint8 _lo, Uint8 _hi) {
    const auto index = Uint16(_lo) << 8 | Uint16(_hi);
    return _program.data[index];
  };

  auto read_imm_vector = [&](Uint8 _lo, Uint8 _hi) {
    const auto index = Uint16(_lo) << 8 | Uint16(_hi);
    return Math::Vec4f {
      _program.data[index + 0],
      _program.data[index + 1],
      _program.data[index + 2],
      _program.data[index + 3]
    };
  };

  const auto& instructions = reinterpret_cast<const Instruction*>(_program.instructions.data());
  const auto n_instructions = _program.instructions.size();
  for (Size i = 0; i < n_instructions; i++) {
    const Instruction& instruction = instructions[i];
    switch (instruction.opcode) {
    case Instruction::OpCode::LI:
      switch (instruction.a.w) {
      case Instruction::Width::SCALAR:
        wr_s(instruction.a.i, read_imm_scalar(instruction.b.u8, instruction.c.u8));
        break;
      case Instruction::Width::VECTOR:
        wr_v(instruction.a.i, read_imm_vector(instruction.b.u8, instruction.c.u8));
        break;
      }
      break;
    // MOV dst:SCALAR, src:SCALAR - Copies src to dst.
    // MOV dst:SCALAR, src:VECTOR - Copies src's component c.i to dst.
    // MOV dst:VECTOR, src:SCALAR - Splats src to all lanes of dst.
    // MOV dst:VECTOR, src:VECTOR - Copies src to dst.
    case Instruction::OpCode::MOV:
      switch (instruction.a.w) {
      case Instruction::Width::SCALAR:
        switch (instruction.b.w) {
        case Instruction::Width::SCALAR:
          // Scalar load -> Scalar store.
          scalar_store(instruction.a, scalar_load(instruction.b));
          break;
        case Instruction::Width::VECTOR:
          // Vector load -> Scalar store.
          scalar_store(instruction.a, vector_load(instruction.b)[instruction.c.i]);
          break;
        }
        break;
      case Instruction::Width::VECTOR:
        switch (instruction.b.w) {
        case Instruction::Width::SCALAR:
          // Scalar load -> Vector store.
          vector_store_s(instruction.a, scalar_load(instruction.b));
          break;
        case Instruction::Width::VECTOR:
          // Vector load -> Vector store.
          vector_store_v(instruction.a, vector_load(instruction.b));
          break;
        }
        break;
      }
      break;
    // All binary operations are defined for:
    //  Scalar <op> Scalar => Scalar
    //  Vector <op> Scalar => Vector
    //  Vector <op> Vector => Vector
    case Instruction::OpCode::ADD:
      BINOP(+);
      break;
    case Instruction::OpCode::SUB:
      BINOP(-);
      break;
    case Instruction::OpCode::MUL:
      BINOP(*);
      break;
    case Instruction::OpCode::MIX:
      // Only valid for a.Width == b.Width && c.Width = SCALAR.
      // Clobbers %s0 for SCALAR or %v0 for VECTOR.
      switch (instruction.a.w) {
      case Instruction::Width::SCALAR:
        wr_s(0, mix(rd_s(instruction.a.i), rd_s(instruction.b.i), rd_s(instruction.c.i)));
        break;
      case Instruction::Width::VECTOR:
        wr_v(0, mix(rd_v(instruction.a.i), rd_v(instruction.b.i), rd_s(instruction.c.i)));
      }
      break;
    case Instruction::OpCode::RND:
      switch (instruction.a.w) {
      case Instruction::Width::SCALAR:
        wr_s(instruction.a.i, rnd(0.0f, 1.0f));
        break;
      case Instruction::Width::VECTOR:
        wr_v(instruction.a.i, Math::Vec4f{rnd(0.0f, 1.0f), rnd(0.0f, 1.0f), rnd(0.0f, 1.0f), rnd(0.0f, 1.0f)});
        break;
      }
      break;

    // These are only valid for a.Width = SCALAR & b.Width = SCALAR.
    case Instruction::OpCode::SIN:
      wr_s(instruction.a.i, Math::sin(rd_s(instruction.b.i)));
      break;
    case Instruction::OpCode::COS:
      wr_s(instruction.a.i, Math::cos(rd_s(instruction.b.i)));
      break;
    case Instruction::OpCode::TAN:
      wr_s(instruction.a.i, Math::tan(rd_s(instruction.b.i)));
      break;
    case Instruction::OpCode::ASIN:
      // TODO(dweiler): Need to implement Math::asin first.
      break;
    case Instruction::OpCode::ACOS:
      wr_s(instruction.a.i, Math::acos(rd_s(instruction.b.i)));
      break;
    case Instruction::OpCode::ATAN:
      wr_s(instruction.a.i, Math::atan(rd_s(instruction.b.i)));
      break;
    }
  }

  return result;
}

} // namespace Rx::Particle