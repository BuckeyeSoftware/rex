#include <string.h> // memset

#include "rx/particle/system.h"
#include "rx/core/concurrency/scope_lock.h"
#include "rx/math/vec4.h"

namespace Rx::Particle {

void System::update(Float32 _delta_time) {
  if (m_alive_count) {
    // Reset acceleration.
    memset(m_acceleration_x, 0, sizeof(Float32) * m_alive_count);
    memset(m_acceleration_y, 0, sizeof(Float32) * m_alive_count);
    memset(m_acceleration_z, 0, sizeof(Float32) * m_alive_count);
  }

  // Execute emitters.
  m_emitters.each_fwd([this, _delta_time](Emitter& _emitter) {
    _emitter.emit(_delta_time, this);
  });

  const auto n_vectors = m_alive_count / 4;
  const auto n_scalars = m_alive_count % 4;

  // Do multiple particles at once.
  auto position_x = reinterpret_cast<Math::Vec4f*>(m_position_x);
  auto position_y = reinterpret_cast<Math::Vec4f*>(m_position_y);
  auto position_z = reinterpret_cast<Math::Vec4f*>(m_position_z);
  auto velocity_x = reinterpret_cast<Math::Vec4f*>(m_velocity_x);
  auto velocity_y = reinterpret_cast<Math::Vec4f*>(m_velocity_y);
  auto velocity_z = reinterpret_cast<Math::Vec4f*>(m_velocity_z);
  auto acceleration_x = reinterpret_cast<Math::Vec4f*>(m_acceleration_x);
  auto acceleration_y = reinterpret_cast<Math::Vec4f*>(m_acceleration_y);
  auto acceleration_z = reinterpret_cast<Math::Vec4f*>(m_acceleration_z);
  auto life = reinterpret_cast<Math::Vec4f*>(m_life);
  const Math::Vec4f delta_time{_delta_time, _delta_time, _delta_time, _delta_time};
  for (Size i = 0; i < n_vectors; i++) {
    position_x[i] = position_x[i] + velocity_x[i] * _delta_time;
    position_y[i] = position_y[i] + velocity_y[i] * _delta_time;
    position_z[i] = position_z[i] + velocity_z[i] * _delta_time;
    // velocity_x[i] = acceleration_x[i] * _delta_time;
    // velocity_y[i] = acceleration_y[i] * _delta_time;
    // velocity_z[i] = acceleration_z[i] * _delta_time;
    life[i] = life[i] - delta_time;
  }

  // TODO(dweiler): Unroll this as it can only be 1, 2, or 3.
  for (Size i = 0; i < n_scalars; i++) {
    const auto index = n_vectors * 4 + i;
    m_position_x[index] = m_position_x[index] + m_velocity_x[index] * _delta_time;
    m_position_y[index] = m_position_y[index] + m_velocity_y[index] * _delta_time;
    m_position_z[index] = m_position_z[index] + m_velocity_z[index] * _delta_time;
    // m_velocity_x[index] = m_acceleration_x[index] * _delta_time;
    // m_velocity_y[index] = m_acceleration_y[index] * _delta_time;
    // m_velocity_z[index] = m_acceleration_z[index] * _delta_time;
    m_life[index] = m_life[index] - _delta_time;
  }

  // Handle killing.
  for (Size i = 0; i < m_alive_count; i++) {
    if (m_life[i] <= 0.0f) {
      kill(i);
    }
  }
}

Optional<Size> System::add_emitter(Emitter&& emitter_) {
  auto index = m_emitters.size();
  if (m_emitters.push_back(Utility::move(emitter_))) {
    return index;
  }
  return nullopt;
}

bool System::remove_emitter(Size _index) {
  if (m_emitters.in_range(_index)) {
    m_emitters.erase(_index, _index + 1);
    return true;
  }
  return false;
}

} // namespace Rx::Particle