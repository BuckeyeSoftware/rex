#ifndef RX_PARTICLE_EMITTER_H
#define RX_PARTICLE_EMITTER_H
#include "rx/particle/program.h"
#include "rx/core/array.h"

namespace Rx::Random { struct Context; }

namespace Rx::Particle {

struct State;

// Emitters spawn particles and generate positions of those spawned particles.
struct Emitter {
  constexpr Emitter(Uint32 _group, const Program* _program, Float32 _rate);

  Emitter(Emitter&& emitter_);
  Emitter& operator=(Emitter&& emitter_);

  /// Emitter parameter control.
  constexpr const Float32& operator[](Size _index) const;
  constexpr Float32& operator[](Size _index);

  /// Retrieve the program associated with this emitter.
  const Program* program() const;

private:
  friend struct System;

  void emit(Random::Context& _random, Float32 _delta_time, State* state_);

  Uint32 m_group;
  const Program* m_program;
  Array<Float32[32]> m_parameters;
  Float32 m_accumulator;
  Float32 m_rate;
};

inline Emitter::Emitter(Emitter&& emitter_)
  : m_group{Utility::exchange(emitter_.m_group, 0)}
  , m_program{Utility::exchange(emitter_.m_program, nullptr)}
  , m_parameters{Utility::move(emitter_.m_parameters)}
  , m_accumulator{Utility::exchange(emitter_.m_accumulator, 0.0f)}
  , m_rate{Utility::exchange(emitter_.m_rate, 0.0f)}
{
}

inline Emitter& Emitter::operator=(Emitter&& emitter_) {
  m_group = Utility::exchange(emitter_.m_group, 0);
  m_program = Utility::exchange(emitter_.m_program, nullptr);
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

inline const Program* Emitter::program() const {
  return m_program;
}

inline constexpr Emitter::Emitter(Uint32 _group, const Program* _program, Float32 _rate)
  : m_group{_group}
  , m_program{_program}
  , m_parameters{}
  , m_accumulator{0.0f}
  , m_rate{_rate}
{
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_EMITTER_H