#ifndef RX_PARTICLE_VM_H
#define RX_PARTICLE_VM_H
#include "rx/core/array.h"

#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

namespace Rx::Particle {

// Bytecode interpreter for particle engine
//
// # Registers
//  Scalar registers (%rs0, %rs1, ..., %rs31)
//  Vector registers (%rv0, %rv1, ..., %rv8)
//
//  Vector registers alias the scalar registers, e.g:
//   rv0 = {rs0,  rs1,  rs2,  rs3}
//   rv1 = {rs4,  rs5,  rs6,  rs7}
//   ...
//   rv8 = {rs28, rs29, rs30, rs31}
//
// # Parameters
//  Scalar parameters (%ps0, %ps1, ..., %ps31)
//  Vector parameters (%pv0, %pv1, ..., %pv8)
//
//  These are "function locals" that can be provided by the user.
//
//  Like regular registers, the vector parameters alias the scalar parameters,
//
//  Parameters cannot be written to.
//
// # Channels
//  Channels can be read and wrote with MOV. When a channel observes a write,
//  the VM updates the particle with the written result.
//
//  Here's a channel table indicating the name, mnemonic, and type
//   | Name          | Mnemonic | Type   |
//   |---------------|----------|--------|
//   | VELOCITY      | %cv0     | VECTOR |
//   | ACCELERATION  | %cv1     | VECTOR |
//   | POSITION      | %cv2     | VECTOR |
//   | COLOR         | %cv3     | VECTOR |
//   | LIFE          | %cs0     | SCALAR |
//   | SIZE          | %cs1     | SCALAR |
//
//  Unlike registers and parameters, channels do not alias.
//
//  Channels %cv4, %cv5, ..., %cv8, as well as;
//  Channels %cs2, %cs3, ..., %cs31 are user-defined channels.
//
// # Instructions
// Each instruction is 32-bits wide and has the following bit encoding:
//  * OpCode  - 8 bits (allows 256 instructions max).
//  * Operand - 2 + 1 + 5 bits (Operand A)
//  * Operand - 2 + 1 + 5 bits (Operand B)
//  * Operand - 2 + 1 + 5 bits (Operand C)
//
// Operand encoding is 8-bits:
//  * Sink  - 2 bits (0 = REGISTER, 1 = PARAMETER, 2 = CHANNEL)
//  * Width - 1 bits (0 = SCALAR, 1 = VECTOR)
//  * Index - 5 bits (allows indexing up to %s31).
struct VM {
  // Cannot index more than 32 parameters due to 5 bit index encoding.
  using Parameters = Array<Float32[32]>;

  enum Channel : Uint8 {
    VELOCITY,
    ACCELERATION,
    POSITION,
    COLOR,
    LIFE,
    SIZE
  };

  struct Instruction {
    enum class OpCode : Uint8 {
      MOV,
      ADD,
      SUB,
      MUL,
      MIX, // Clobbers s0 in scalar, v0 in vector.
      RND,
      SIN,
      COS,
      TAN,
      ASIN,
      ACOS,
      ATAN
    };

    enum class Sink : Uint8 {
      REGISTER,
      PARAMETER,
      CHANNEL
    };

    enum class Width : Uint8 {
      SCALAR,
      VECTOR
    };

    OpCode opcode;

    struct Operand {
      Sink s : 2;
      Width w : 1;
      Uint8 i : 5;
    };

    union {
      struct {
        Operand a;
        Operand b;
        Operand c;
      };
      Operand ops[3];
    };
  };
  static_assert(sizeof(Instruction) == 4);

  struct Result {
    // Keep these in the same order as Channel enum.
    Math::Vec4f velocity;
    Math::Vec4f acceleration;
    Math::Vec4f position;
    Math::Vec4f color;
    Float32 life;
    Float32 size;
    Uint8 mask;
  };

  Result execute(const Parameters& _parameters,
    const Instruction* _instructions, Size _n_instructions);

private:
  union {
    Float32 s[32];
    Float32 v[8][4];
  } m_registers;
};

} // namespace Rx::Particle

#endif // RX_PARTICLE_VM_H