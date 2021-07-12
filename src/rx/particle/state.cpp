#include "rx/particle/state.h"

#include "rx/core/memory/aggregate.h"
#include "rx/core/memory/copy.h"
#include "rx/core/memory/zero.h"

#include "rx/core/utility/swap.h"

#include "rx/core/concurrency/scope_lock.h"

#include "rx/math/frustum.h"

namespace Rx::Particle {

void State::kill(Uint32 _index) {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  swap(_index, m_alive_count - 1);
  m_alive_count--;
}

void State::spawn(Uint32 _group, Uint32 _index) {
  RX_ASSERT(_group < m_group_count, "out of bounds");

  swap(_index, m_alive_count);
  // Count another particle in this group.
  m_group_data[_group].count++;
  // Remember the group the particle is in.
  m_group_refs[m_alive_count] = _group;
  m_alive_count++;
}

void State::swap(Uint32 _lhs, Uint32 _rhs) {
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

  swap(m_texture[_lhs], m_texture[_rhs]);

  swap(m_group_refs[_lhs], m_group_refs[_rhs]);
}

bool State::resize(Size _particles, Size _groups) {
  // Round |_particles| to a multiple of 16 because Memory::BumpPointAllocator
  // requires it for the indices allocator size and because the particle engine
  // uses SoA approach to do multiple particles at a time with SIMD.
  _particles = (_particles + 15) & -16;

  // Nothing to resize.
  if (_particles <= m_total_count && _groups <= m_group_count) {
    return true;
  }

  bool result = true;
  Memory::Aggregate aggregate;

  // The first allocation on the aggregate will be Memory::ALIGNMENT aligned,
  // use that for the BumpPointAllocator which requires that alignment too. This
  // can be done in a later aggregate.add but it could involve wasting a few
  // bytes to pad for that alignment.
  result &= aggregate.add(sizeof(Uint32), Memory::Allocator::ALIGNMENT, _particles);

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

  result &= aggregate.add<Uint16>(_particles); // m_texture

  result &= aggregate.add<Uint32>(_particles); // m_group_refs

  result &= aggregate.add<Group>(_groups); // m_groups

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

    memcpy(data + aggregate[9], m_color_r, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[10], m_color_g, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[11], m_color_b, sizeof(Float32) * m_alive_count);
    memcpy(data + aggregate[12], m_color_a, sizeof(Float32) * m_alive_count);

    memcpy(data + aggregate[13], m_life, sizeof(Float32) * m_alive_count);

    memcpy(data + aggregate[14], m_size, sizeof(Float32) * m_alive_count);

    memcpy(data + aggregate[15], m_texture, sizeof(Uint16) * m_alive_count);
  }
#endif

  m_allocator.deallocate(m_data);

  m_data = data;

  // The indices allocator now has access to _particles worth of Uint32.
  m_indices_allocator.init(data + aggregate[0], sizeof(Uint32) * _particles);

  m_velocity_x     = reinterpret_cast<Float32*>(data + aggregate[1]);
  m_velocity_y     = reinterpret_cast<Float32*>(data + aggregate[2]);
  m_velocity_z     = reinterpret_cast<Float32*>(data + aggregate[3]);

  m_acceleration_x = reinterpret_cast<Float32*>(data + aggregate[4]);
  m_acceleration_y = reinterpret_cast<Float32*>(data + aggregate[5]);
  m_acceleration_z = reinterpret_cast<Float32*>(data + aggregate[6]);

  m_position_x     = reinterpret_cast<Float32*>(data + aggregate[7]);
  m_position_y     = reinterpret_cast<Float32*>(data + aggregate[8]);
  m_position_z     = reinterpret_cast<Float32*>(data + aggregate[9]);

  m_color_r        = reinterpret_cast<Float32*>(data + aggregate[10]);
  m_color_g        = reinterpret_cast<Float32*>(data + aggregate[11]);
  m_color_b        = reinterpret_cast<Float32*>(data + aggregate[12]);
  m_color_a        = reinterpret_cast<Float32*>(data + aggregate[13]);

  m_life           = reinterpret_cast<Float32*>(data + aggregate[14]);

  m_size           = reinterpret_cast<Float32*>(data + aggregate[15]);

  m_texture        = reinterpret_cast<Uint16*>(data + aggregate[16]);

  m_group_refs     = reinterpret_cast<Uint32*>(data + aggregate[17]);

  m_group_data     = reinterpret_cast<Group*>(data + aggregate[18]);

  m_group_count    = _groups;
  m_total_count    = _particles;

  return true;
}

Math::Vec3f State::position(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  const auto x = m_position_x[_index];
  const auto y = m_position_y[_index];
  const auto z = m_position_z[_index];
  return {x, y, z};
}

Math::Vec4b State::color(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  const auto r = Byte(m_color_r[_index] * 255.0);
  const auto g = Byte(m_color_g[_index] * 255.0);
  const auto b = Byte(m_color_b[_index] * 255.0);
  const auto a = Byte(m_color_a[_index] * 255.0);
  return {r, g, b, a};
}

Float32 State::size(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  return m_size[_index];
}

Uint16 State::texture(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  return m_texture[_index];
}

State::~State() {
  m_allocator.deallocate(m_data);
}

Size State::visible(Span<Uint32> indices_, const Math::Frustum& _frustum) const {
  Size count = 0;
  for (Size i = 0; i < m_group_count; i++) {
    const auto& group = m_group_data[i];
    if (group.count && _frustum.is_aabb_inside(group.bounds)) {
      Memory::copy(indices_.data() + count, group.indices, group.count);
      count += group.count;
    }
  }
  return count;
}

} // namespace Rx::Particle