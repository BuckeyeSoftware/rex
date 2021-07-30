#ifndef RX_PARTICLE_STATE_H
#define RX_PARTICLE_STATE_H
#include "rx/core/memory/bump_point_allocator.h"
#include "rx/core/uninitialized.h"

#include "rx/math/aabb.h"
#include "rx/math/vec4.h"

namespace Rx::Math { struct Frustum; }

namespace Rx::Particle {

// State is thread-safe.
struct State {
  RX_MARK_NO_COPY(State);
  RX_MARK_NO_MOVE(State);

  struct Group {
    Math::AABB bounds;
    Uint32* indices;
    Uint32 count;
  };

  State(Memory::Allocator& _allocator);
  ~State();

  [[nodiscard]] bool resize(Size _particles, Size _groups);

  void spawn(Uint32 _group, Uint32 _index);
  void kill(Uint32 _index);

  Size alive_count() const;
  Size total_count() const;

  Math::Vec3f position(Uint32 _index) const;
  Math::Vec4b color(Uint32 _index) const;
  Float32 size(Uint32 _index) const;
  Uint16 texture(Uint32 _index) const;

  // Check for all visible particles in the frustum. This expects |indices_|
  // contains enough space for every alive particle. Returns the number of
  // visible particles.
  Size visible(Span<Uint32> indices_, const Math::Frustum& _frustum) const;

  Uint64 id() const;

  Span<const Group> groups() const;

protected:
  friend struct Emitter;
  friend struct System;

  void swap(Uint32 _lhs, Uint32 _rhs);

  Memory::Allocator& m_allocator;

  // TODO(dweiler): Use Optional?
  Uninitialized<Memory::BumpPointAllocator> m_indices_allocator;

  Byte* m_data;

  Size m_alive_count;
  Size m_total_count;

  Float32* m_velocity_x;
  Float32* m_velocity_y;
  Float32* m_velocity_z;

  Float32* m_acceleration_x;
  Float32* m_acceleration_y;
  Float32* m_acceleration_z;

  Float32* m_position_x;
  Float32* m_position_y;
  Float32* m_position_z;

  Float32* m_color_r;
  Float32* m_color_g;
  Float32* m_color_b;
  Float32* m_color_a;

  Float32* m_life;
  Float32* m_size;

  Uint16* m_texture;

  Uint64 m_id;

  Uint32* m_group_refs;

  // The group array as referenced by index in |m_group_refs|.
  Group* m_group_data;
  Size m_group_count;
};

inline State::State(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{nullptr}
  , m_alive_count{0}
  , m_total_count{0}
  , m_velocity_x{nullptr}
  , m_velocity_y{nullptr}
  , m_velocity_z{nullptr}
  , m_acceleration_x{nullptr}
  , m_acceleration_y{nullptr}
  , m_acceleration_z{nullptr}
  , m_position_x{nullptr}
  , m_position_y{nullptr}
  , m_position_z{nullptr}
  , m_color_r{nullptr}
  , m_color_g{nullptr}
  , m_color_b{nullptr}
  , m_color_a{nullptr}
  , m_life{nullptr}
  , m_size{nullptr}
  , m_texture{nullptr}
  , m_id{0}
  , m_group_refs{nullptr}
  , m_group_data{nullptr}
  , m_group_count{0}
{
}

inline Size State::alive_count() const {
  return m_alive_count;
}

inline Size State::total_count() const {
  return m_total_count;
}

RX_HINT_FORCE_INLINE Math::Vec3f State::position(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  const auto x = m_position_x[_index];
  const auto y = m_position_y[_index];
  const auto z = m_position_z[_index];
  return {x, y, z};
}

RX_HINT_FORCE_INLINE Math::Vec4b State::color(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  const auto r = Byte(m_color_r[_index] * 255.0);
  const auto g = Byte(m_color_g[_index] * 255.0);
  const auto b = Byte(m_color_b[_index] * 255.0);
  const auto a = Byte(m_color_a[_index] * 255.0);
  return {r, g, b, a};
}

RX_HINT_FORCE_INLINE Float32 State::size(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  return m_size[_index];
}

RX_HINT_FORCE_INLINE Uint16 State::texture(Uint32 _index) const {
  RX_ASSERT(_index < m_alive_count, "out of bounds");
  return m_texture[_index];
}

inline Uint64 State::id() const {
  return m_id;
}

inline Span<const State::Group> State::groups() const {
  return { m_group_data, m_group_count };
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_STATE_H