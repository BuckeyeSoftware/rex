#ifndef RX_PARTICLE_PROGRAM_H
#define RX_PARTICLE_PROGRAM_H
#include "rx/core/vector.h"

namespace Rx::Particle {

struct Program {
  static Optional<Program> copy(const Program& _program);

  Vector<Uint32> instructions;
  Vector<Float32> data;
};

} // namespace Rx::Particle

#endif // RX_PARTICLE_PROGRAM_H