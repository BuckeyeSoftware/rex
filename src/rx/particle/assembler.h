#ifndef RX_PARTICLE_ASSEMBLER_H
#define RX_PARTICLE_ASSEMBLER_H
#include "rx/core/string.h"

namespace Rx::Particle {

struct Assembler {
  // Assembles particle assembly source file |_src_file|.
  [[nodiscard]] bool assemble(const String& _src_file);

  // Read the assembled instructions.
  const Vector<Uint32>& instructions() const &;

  // When |assemble| returns false the error will be here.
  const String& error() const &;

private:
  Vector<Uint32> m_instructions;
  String m_error;
};

inline const Vector<Uint32>& Assembler::instructions() const & {
  return m_instructions;
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_ASSEMBLER_H