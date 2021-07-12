#ifndef RX_PARTICLE_SYSTEM_H
#define RX_PARTICLE_SYSTEM_H
#include "rx/particle/state.h"
#include "rx/particle/emitter.h"

#include "rx/core/vector.h"
#include "rx/core/string.h"
#include "rx/core/map.h"

#include "rx/core/random/mersenne_twister.h"

#include "rx/core/memory/temporary_allocator.h"

namespace Rx { struct StringView; }

namespace Rx::Particle {

struct System
  : State
{
  System(Memory::Allocator& _allocator);

  void update(Float32 _delta_time);

  [[nodiscard]] Optional<Size> add_emitter(Uint32 _group, const Program& _program, Float32 _rate);
  [[nodiscard]] bool add_texture(const StringView& _file_name);

  Emitter& emitter(Size _index);
  const Emitter& emitter(Size _index) const;

private:
  Memory::TemporaryAllocator<1_MiB> m_allocator;
  Random::MersenneTwister m_random;
  Vector<Emitter> m_emitters;
  Map<Program::Hash, Program> m_programs;
};

inline System::System(Memory::Allocator& _allocator)
  : State{_allocator}
  , m_allocator{_allocator}
  , m_emitters{m_allocator}
  , m_programs{m_allocator}
{
}

inline Emitter& System::emitter(Size _index) {
  return m_emitters[_index];
}

inline const Emitter& System::emitter(Size _index) const {
  return m_emitters[_index];
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_SYSTEM_H