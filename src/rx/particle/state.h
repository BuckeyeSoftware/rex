#ifndef RX_PARTICLE_STATE_H
#define RX_PARTICLE_STATE_H
#include "rx/core/memory/allocator.h"

#include "rx/math/vec3.h"
#include "rx/math/vec4.h"

namespace Rx::Particle {

// State is thread-safe.
struct State {
  RX_MARK_NO_COPY(State);
  RX_MARK_NO_MOVE(State);

  constexpr State(Memory::Allocator& _allocator);
  ~State();

  [[nodiscard]] bool resize(Size _particles);

  bool spawn(Size _index);
  bool kill(Size _index);

  Size alive_count() const;
  Size total_count() const;

  Math::Vec3f position(Size _index) const;
  Math::Vec4f color(Size _index) const;
  Float32 size(Size _index) const;

protected:
  friend struct Emitter;

  void swap(Size _lhs, Size _rhs);

  Memory::Allocator& m_allocator;

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
};

inline constexpr State::State(Memory::Allocator& _allocator)
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
{
}

inline Size State::alive_count() const {
  return m_alive_count;
}

inline Size State::total_count() const {
  return m_total_count;
}

} // namespace Rx::Particle

#endif // RX_PARTICLE_STATE_H