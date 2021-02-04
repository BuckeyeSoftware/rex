#ifndef RX_PARTICLE_EMITTER_H
#define RX_PARTICLE_EMITTER_H
#include "rx/core/vector.h"
#include "rx/particle/vm.h"

namespace Rx::Particle {

struct State;

// Emitters spawn particles and generate positions of those spawned particles.
struct Emitter {
  RX_MARK_NO_COPY(Emitter);

  using Instruction = VM::Instruction;

  Emitter(Vector<Instruction>&& instructions_, Float32 _rate);
  Emitter(Emitter&& emitter_);
  Emitter& operator=(Emitter&& emitter_);

  void emit(Float32 _delta_time, State* state_);

  // Emitter parameter control.
  constexpr const Float32& operator[](Size _index) const;
  constexpr Float32& operator[](Size _index);

private:
  Vector<Instruction> m_instructions;
  Array<Float32[32]> m_parameters;
  Float32 m_accumulator;
  Float32 m_rate;
};

inline Emitter::Emitter(Emitter&& emitter_)
  : m_instructions{Utility::move(emitter_.m_instructions)}
  , m_parameters{Utility::move(emitter_.m_parameters)}
  , m_accumulator{Utility::exchange(emitter_.m_accumulator, 0.0f)}
  , m_rate{Utility::exchange(emitter_.m_rate, 0.0f)}
{
}

inline Emitter& Emitter::operator=(Emitter&& emitter_) {
  m_instructions = Utility::move(emitter_.m_instructions);
  m_parameters = Utility::move(emitter_.m_parameters);
  m_accumulator = Utility::exchange(emitter_.m_accumulator, 0.0f);
  m_rate = Utility::exchange(emitter_.m_rate, 0.0f);
  return *this;
}

inline constexpr const Float32& Emitter::operator[](Size _index) const {
  return m_parameters[_index];
}

inline constexpr Float32& Emitter::operator[](Size _index) {
  return m_parameters[_index];
}

inline Emitter::Emitter(Vector<Instruction>&& instructions_, Float32 _rate)
  : m_instructions{Utility::move(instructions_)}
  , m_accumulator{0.0f}
  , m_rate{_rate}
{
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_EMITTER_H