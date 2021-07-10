#include "rx/particle/state.h"

#include "rx/core/memory/aggregate.h"
#include "rx/core/memory/copy.h"
#include "rx/core/memory/zero.h"

#include "rx/core/utility/swap.h"

#include "rx/core/concurrency/scope_lock.h"

namespace Rx::Particle {

bool State::kill(Size _index) {
  if (m_alive_count == 0) {
    return false;
  }

  swap(_index, m_alive_count - 1);

  m_alive_count--;

  return true;
}

bool State::spawn(Size _index) {
  if (m_alive_count >= m_total_count) {
    return false;
  }

  swap(_index, m_alive_count);

  m_alive_count++;

  return true;
}

void State::swap(Size _lhs, Size _rhs) {
  using Utility::swap;

  swap(m_velocity_x[_lhs], m_velocity_x[_rhs]);
  swap(m_velocity_y[_lhs], m_velocity_y[_rhs]);
  swap(m_velocity_z[_lhs], m_velocity_z[_rhs]);

  swap(m_acceleration_x[_lhs], m_acceleration_x[_rhs]);
  swap(m_acceleration_y[_lhs], m_acceleration_y[_rhs]);
  swap(m_acceleration_z[_lhs], m_acceleration_z[_rhs]);

  swap(m_position_x[_lhs], m_position_x[_rhs]);
  swap(m_position_y[_lhs], m_position_y[_rhs]);
  swap(m_position_z[_lhs], m_position_z[_rhs]);

  swap(m_life[_lhs], m_life[_rhs]);

  swap(m_size[_lhs], m_size[_rhs]);
}

bool State::resize(Size _particles) {
  // Round |_particles| to a multiple of four because of SIMD.
  _particles = (_particles + 3) & -4;

  if (_particles <= m_total_count) {
    return true;
  }

  bool result = true;
  Memory::Aggregate aggregate;
  result &= aggregate.add<Float32>(_particles); // m_velocity_x
  result &= aggregate.add<Float32>(_particles); // m_velocity_y
  result &= aggregate.add<Float32>(_particles); // m_velocity_z

  result &= aggregate.add<Float32>(_particles); // m_acceleration_x
  result &= aggregate.add<Float32>(_particles); // m_acceleration_y
  result &= aggregate.add<Float32>(_particles); // m_acceleration_z

  result &= aggregate.add<Float32>(_particles); // m_color_r
  result &= aggregate.add<Float32>(_particles); // m_color_g
  result &= aggregate.add<Float32>(_particles); // m_color_b
  result &= aggregate.add<Float32>(_particles); // m_color_a

  result &= aggregate.add<Float32>(_particles); // m_position_x
  result &= aggregate.add<Float32>(_particles); // m_position_y
  result &= aggregate.add<Float32>(_particles); // m_position_z

  result &= aggregate.add<Float32>(_particles); // m_life

  result &= aggregate.add<Float32>(_particles); // m_size

  result &= aggregate.finalize();

  if (!result) {
    return false;
  }

  Byte* data = m_allocator.allocate(aggregate.bytes());
  if (!data) {
    return false;
  }

  // TODO(dweiler): Should the existing particles be copied on resize?
#if 0
  // Zero fill...
  Memory::zero(data, aggregate.bytes());

  if (m_alive_count) {
    memcpy(data + aggregate[0], m_velocity_x, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[1], m_velocity_y, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[2], m_velocity_z, sizeof(Float32) * m_alive_count);

    memcpy(data + aggregate[3], m_acceleration_x, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[4], m_acceleration_y, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[5], m_acceleration_z, sizeof(Float32) * m_alive_count);

    memcpy(data + aggregate[6], m_position_x, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[7], m_position_y, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[8], m_position_z, sizeof(Float32) * m_alive_count);

    memcpy(data + aggregate[9], m_life, sizeof(Float32) * m_alive_count);

    memcpy(data + aggregate[10], m_size, sizeof(Float32) * m_alive_count);
  }
#endif

  m_allocator.deallocate(m_data);
  m_data = data;

  m_velocity_x     = reinterpret_cast<Float32*>(data + aggregate[0]);
  m_velocity_y     = reinterpret_cast<Float32*>(data + aggregate[1]);
  m_velocity_z     = reinterpret_cast<Float32*>(data + aggregate[2]);

  m_acceleration_x = reinterpret_cast<Float32*>(data + aggregate[3]);
  m_acceleration_y = reinterpret_cast<Float32*>(data + aggregate[4]);
  m_acceleration_z = reinterpret_cast<Float32*>(data + aggregate[5]);

  m_position_x     = reinterpret_cast<Float32*>(data + aggregate[6]);
  m_position_y     = reinterpret_cast<Float32*>(data + aggregate[7]);
  m_position_z     = reinterpret_cast<Float32*>(data + aggregate[8]);

  m_color_r        = reinterpret_cast<Float32*>(data + aggregate[9]);
  m_color_g        = reinterpret_cast<Float32*>(data + aggregate[10]);
  m_color_b        = reinterpret_cast<Float32*>(data + aggregate[11]);
  m_color_a        = reinterpret_cast<Float32*>(data + aggregate[12]);

  m_life           = reinterpret_cast<Float32*>(data + aggregate[13]);

  m_size           = reinterpret_cast<Float32*>(data + aggregate[14]);

  m_total_count = _particles;

  return true;
}

Math::Vec3f State::position(Size _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  const auto x = m_position_x[_index];
  const auto y = m_position_y[_index];
  const auto z = m_position_z[_index];
  return {x, y, z};
}

Math::Vec4f State::color(Size _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  const auto r = m_color_r[_index];
  const auto g = m_color_g[_index];
  const auto b = m_color_b[_index];
  const auto a = m_color_a[_index];
  return {r, g, b, a};
}

Float32 State::size(Size _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  return m_size[_index];
}

State::~State() {
  m_allocator.deallocate(m_data);
}

} // namespace Rx::Particle