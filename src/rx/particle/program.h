#ifndef RX_PARTICLE_PROGRAM_H
#define RX_PARTICLE_PROGRAM_H
#include "rx/core/vector.h"
#include "rx/core/array.h"

namespace Rx::Particle {

struct Program {
  static Optional<Program> copy(const Program& _program);

  using Hash = Array<Byte[16]>;
  Vector<Uint32> instructions;
  Vector<Float32> data;
  Hash hash;
};

} // namespace Rx::Particle

#endif // RX_PARTICLE_PROGRAM_H