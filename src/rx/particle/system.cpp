#include "rx/particle/system.h"
#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/memory/zero.h"
#include "rx/math/vec4.h"

namespace Rx::Particle {

void System::update(Float32 _delta_time) {
  auto& indices_allocator = *m_indices_allocator.data();

  if (m_alive_count) {
    // Reset acceleration.
    Memory::zero(m_acceleration_x, m_alive_count);
    Memory::zero(m_acceleration_y, m_alive_count);
    Memory::zero(m_acceleration_z, m_alive_count);

    // Reset the indices allocator.
    indices_allocator.reset();
  }

  // Execute emitters.
  m_emitters.each_fwd([this, _delta_time](Emitter& _emitter) {
    _emitter.emit(m_random, _delta_time, this);
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
  // auto acceleration_x = reinterpret_cast<Math::Vec4f*>(m_acceleration_x);
  // auto acceleration_y = reinterpret_cast<Math::Vec4f*>(m_acceleration_y);
  // auto acceleration_z = reinterpret_cast<Math::Vec4f*>(m_acceleration_z);
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

  // Unrolled for remainder.
  Size index = 0;
  switch (n_scalars) {
  case 3:
    index = n_vectors * 4 + 2;
    m_position_x[index] = m_position_x[index] + m_velocity_x[index] * _delta_time;
    m_position_y[index] = m_position_y[index] + m_velocity_y[index] * _delta_time;
    m_position_z[index] = m_position_z[index] + m_velocity_z[index] * _delta_time;
    // m_velocity_x[index] = m_acceleration_x[index] * _delta_time;
    // m_velocity_y[index] = m_acceleration_y[index] * _delta_time;
    // m_velocity_z[index] = m_acceleration_z[index] * _delta_time;
    m_life[index] = m_life[index] - _delta_time;
    [[fallthrough]];
  case 2:
    index = n_vectors * 4 + 1;
    m_position_x[index] = m_position_x[index] + m_velocity_x[index] * _delta_time;
    m_position_y[index] = m_position_y[index] + m_velocity_y[index] * _delta_time;
    m_position_z[index] = m_position_z[index] + m_velocity_z[index] * _delta_time;
    // m_velocity_x[index] = m_acceleration_x[index] * _delta_time;
    // m_velocity_y[index] = m_acceleration_y[index] * _delta_time;
    // m_velocity_z[index] = m_acceleration_z[index] * _delta_time;
    m_life[index] = m_life[index] - _delta_time;
    [[fallthrough]];
  case 1:
    index = n_vectors * 4;
    m_position_x[index] = m_position_x[index] + m_velocity_x[index] * _delta_time;
    m_position_y[index] = m_position_y[index] + m_velocity_y[index] * _delta_time;
    m_position_z[index] = m_position_z[index] + m_velocity_z[index] * _delta_time;
    // m_velocity_x[index] = m_acceleration_x[index] * _delta_time;
    // m_velocity_y[index] = m_acceleration_y[index] * _delta_time;
    // m_velocity_z[index] = m_acceleration_z[index] * _delta_time;
    m_life[index] = m_life[index] - _delta_time;
    break;
  }

  // Handle killing of particles.
  for (Size i = 0; i < m_alive_count; i++) {
    if (m_life[i] <= 0.0f) {
      kill(i);
    }
  }

  // Reset each group bounds and allocate arrays for indices.
  for (Size i = 0; i < m_group_count; i++) {
    auto& group = m_group_data[i];

    // Skip empty groups.
    if (group.count == 0) {
      continue;
    }

    group.bounds.reset();

    // This allocation cannot fail.
    auto indices = indices_allocator.allocate(sizeof(Uint32), group.count);
    group.indices = reinterpret_cast<Uint32*>(indices);

    // Repurpose this counter as an index when adding indices below.
    group.count = 0;
  };

  // Append each particle to the appropriate group.
  for (Size i = 0; i < m_alive_count; i++) {
    auto& group = m_group_data[m_group_refs[i]];
    group.bounds.expand(position(i));
    group.indices[group.count++] = i;
  }

  // Unique id for each update.
  m_id++;
}

Optional<Size> System::add_emitter(Uint32 _group, const Program& _program, Float32 _rate) {
  // System does not have that many groups.
  if (_group >= m_group_count) {
    return nullopt;
  }
  // Reuse the program in cache if found.
  if (auto program = m_programs.find(_program.hash)) {
    const auto index = m_emitters.size();
    if (m_emitters.emplace_back(_group, program, _rate)) {
      return index;
    }
  } else if (auto copy = Utility::copy(_program)) {
    if (m_programs.insert(_program.hash, Utility::move(*copy))) {
      // Retry now that the program is copied and added to the system.
      return add_emitter(_group, _program, _rate);
    }
  }
  return nullopt;
}

} // namespace Rx::Particle