#include "rx/particle/emitter.h"
#include "rx/particle/state.h"
#include "rx/particle/vm.h"

#include "rx/core/math/mod.h"

namespace Rx::Particle {

void Emitter::emit(Random::Context& _random, Float32 _delta_time, State* state_) {
  using Channel = VM::Channel;

  m_accumulator += _delta_time;
  const auto offset = Size(m_accumulator * m_rate);
  const auto beg = state_->m_alive_count;
  const auto end = Algorithm::min(beg + offset, state_->m_total_count ? state_->m_total_count - 1 : 0);

  // TODO(dweiler): Revisit rate / accumulator??
  m_accumulator = Math::mod(m_accumulator, 1.0f / m_rate);

  // TODO(dweiler): Execute on thread pool.
  VM vm;
  for (Size i = beg; i < end; i++) {
    state_->spawn(m_group, i);

    const auto result = vm.execute(_random, m_parameters, *m_program);

    // Check the execution mask to know which channels to update.
    if (result.mask & (1 << Channel::VELOCITY)) {
      state_->m_velocity_x[i] = result.velocity.x;
      state_->m_velocity_y[i] = result.velocity.y;
      state_->m_velocity_z[i] = result.velocity.z;
    }

    if (result.mask & (1 << Channel::ACCELERATION)) {
      state_->m_acceleration_x[i] = result.acceleration.x;
      state_->m_acceleration_y[i] = result.acceleration.y;
      state_->m_acceleration_z[i] = result.acceleration.z;
    }

    if (result.mask & (1 << Channel::POSITION)) {
      state_->m_position_x[i] = result.position.x;
      state_->m_position_y[i] = result.position.y;
      state_->m_position_z[i] = result.position.z;
    }

    if (result.mask & (1 << Channel::COLOR)) {
      state_->m_color_r[i] = result.color.r;
      state_->m_color_g[i] = result.color.g;
      state_->m_color_b[i] = result.color.b;
      state_->m_color_a[i] = result.color.a;
    }

    if (result.mask & (1 << Channel::LIFE)) {
      state_->m_life[i] = result.life;
    }

    if (result.mask & (1 << Channel::SIZE)) {
      state_->m_size[i] = result.size;
    }
  }

  // state_->m_tree.validate();
}

} // namespace Rx::Particle

