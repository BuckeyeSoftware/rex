#ifndef RX_PARTICLE_ASSEMBLER_H
#define RX_PARTICLE_ASSEMBLER_H
#include "rx/particle/program.h"
#include "rx/core/string.h"

namespace Rx::Particle {

struct Assembler {
  Assembler(Memory::Allocator& _allocator)
    : m_error{_allocator}
  {
  }

  // Assembles particle assembly source file |_src_file| and produces a Program.
  [[nodiscard]] Optional<Program> assemble(const StringView& _src_file);

  // When |assemble| returns nullopt the error will be here.
  const String& error() const &;

private:
  String m_error;
};

inline const String& Assembler::error() const & {
  return m_error;
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_ASSEMBLER_H