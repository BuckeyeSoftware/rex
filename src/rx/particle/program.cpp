#include "rx/particle/program.h"

namespace Rx::Particle {

Optional<Program> Program::copy(const Program& _program) {
  auto instructions = Utility::copy(_program.instructions);
  auto data = Utility::copy(_program.data);
  if (!instructions || !data) {
    return nullopt;
  }
  return Program{Utility::move(*instructions), Utility::move(*data)};
}

} // namespace Rx::Particle