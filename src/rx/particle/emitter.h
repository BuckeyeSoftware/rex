#ifndef RX_PARTICLE_EMITTER_H
#define RX_PARTICLE_EMITTER_H
#include "rx/particle/program.h"
#include "rx/core/array.h"

namespace Rx::Particle {

struct State;

// Emitters spawn particles and generate positions of those spawned particles.
struct Emitter {
  RX_MARK_NO_COPY(Emitter);

  static Optional<Emitter> create(const Program& _program, Float32 _rate);

  Emitter(Emitter&& emitter_);
  Emitter& operator=(Emitter&& emitter_);

  void emit(Float32 _delta_time, State* state_);

  // Emitter parameter control.
  constexpr const Float32& operator[](Size _index) const;
  constexpr Float32& operator[](Size _index);

private:
  Emitter(Program&& program_, Float32 _rate);

  Program m_program;
  Array<Float32[32]> m_parameters;
  Float32 m_accumulator;
  Float32 m_rate;
};

inline Emitter::Emitter(Emitter&& emitter_)
  : m_program{Utility::move(emitter_.m_program)}
  , m_parameters{Utility::move(emitter_.m_parameters)}
  , m_accumulator{Utility::exchange(emitter_.m_accumulator, 0.0f)}
  , m_rate{Utility::exchange(emitter_.m_rate, 0.0f)}
{
}

inline Emitter& Emitter::operator=(Emitter&& emitter_) {
  m_program = Utility::move(emitter_.m_program);
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

inline Emitter::Emitter(Program&& program_, Float32 _rate)
  : m_program{Utility::move(program_)}
  , m_accumulator{0.0f}
  , m_rate{_rate}
{
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_EMITTER_H