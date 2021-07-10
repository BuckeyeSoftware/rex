#ifndef RX_PARTICLE_SYSTEM_H
#define RX_PARTICLE_SYSTEM_H
#include "rx/particle/state.h"
#include "rx/particle/emitter.h"

#include "rx/core/vector.h"

#include "rx/core/random/mersenne_twister.h"

namespace Rx::Particle {

struct System : State {
  constexpr System(Memory::Allocator& _allocator);

  void update(Float32 _delta_time);

  // Insert |emitter|.
  [[nodiscard]] Optional<Size> insert(Emitter&& emitter_);

  Emitter& operator[](Size _index) &;
  const Emitter& operator[](Size _index) const &;

private:
  Random::MersenneTwister m_random;
  Vector<Emitter> m_emitters;
};

inline constexpr System::System(Memory::Allocator& _allocator)
  : State{_allocator}
  , m_emitters{_allocator}
{
}

inline Emitter& System::operator[](Size _index) & {
  return m_emitters[_index];
}

inline const Emitter& System::operator[](Size _index) const & {
  return m_emitters[_index];
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_SYSTEM_H