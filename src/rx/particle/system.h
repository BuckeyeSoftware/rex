#ifndef RX_PARTICLE_SYSTEM_H
#define RX_PARTICLE_SYSTEM_H
#include "rx/particle/state.h"
#include "rx/particle/emitter.h"

#include "rx/core/vector.h"

namespace Rx::Particle {

struct System : State {
  constexpr System(Memory::Allocator& _allocator);

  void update(Float32 _delta_time);

  // Insert |emitter|.
  [[nodiscard]] Optional<Size> add_emitter(Emitter&& emitter_);
  [[nodiscard]] bool remove_emitter(Size _index);

private:
  Vector<Emitter> m_emitters;
};

inline constexpr System::System(Memory::Allocator& _allocator)
  : State{_allocator}
{
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_SYSTEM_H