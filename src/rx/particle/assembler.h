#ifndef RX_PARTICLE_ASSEMBLER_H
#define RX_PARTICLE_ASSEMBLER_H
#include "rx/particle/program.h"
#include "rx/core/string.h"

namespace Rx::Particle {

struct Assembler {
  Assembler(Memory::Allocator& _allocator)
    : m_program{_allocator, _allocator}
    , m_error{_allocator}
  {
  }

  // Assembles particle assembly source file |_src_file|.
  [[nodiscard]] bool assemble(const String& _src_file);

  const Program& program() const &;

  // When |assemble| returns false the error will be here.
  const String& error() const &;

private:
  Program m_program;
  String m_error;
};

inline const Program& Assembler::program() const & {
  return m_program;
}

inline const String& Assembler::error() const & {
  return m_error;
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_ASSEMBLER_H