#ifndef RX_RENDER_PARTICLE_SYSTEM_H
#define RX_RENDER_PARTICLE_SYSTEM_H
#include "rx/math/vec3.h"
#include "rx/math/mat4x4.h"

#include "rx/core/vector.h"

namespace Rx::Particle {
  struct System;
} // namespace Rx::Particle

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Buffer;
  struct Technique;
  struct Target;
} // namespace Frontend

struct ParticleSystem {
  RX_MARK_NO_COPY(ParticleSystem);

  constexpr ParticleSystem();
  ParticleSystem(ParticleSystem&& particle_system_);
  ~ParticleSystem();

  ParticleSystem& operator=(ParticleSystem&& particle_system_);

  static Optional<ParticleSystem> create(Frontend::Context* _frontend);

  void render(const Particle::System* _system, Frontend::Target* _target,
    const Math::Mat4x4f& _model, const Math::Mat4x4f& _view,
    const Math::Mat4x4f& _projection);

private:

  constexpr ParticleSystem(
    Frontend::Context* _frontend, Frontend::Buffer* _buffer,
    Frontend::Technique* _technique);

  void release();

  // Keeping this structure small is important for bandwidth purposes.
  struct Vertex {
    Math::Vec3f position;
    Float32 size;
    Math::Vec4b color;
  };

  Frontend::Context* m_frontend;
  Frontend::Buffer* m_buffer;
  Frontend::Technique* m_technique;
  Uint64 m_last_id;
  Size m_count;

  Vector<Uint32> m_indices;
};

inline constexpr ParticleSystem::ParticleSystem()
  : ParticleSystem{nullptr, nullptr, nullptr}
{
}

inline constexpr ParticleSystem::ParticleSystem(
  Frontend::Context* _frontend, Frontend::Buffer* _buffer,
  Frontend::Technique* _technique)
  : m_frontend{_frontend}
  , m_buffer{_buffer}
  , m_technique{_technique}
  , m_last_id{0}
  , m_count{0}
{
}

inline ParticleSystem::ParticleSystem(ParticleSystem&& particle_system_)
  : m_frontend{Utility::exchange(particle_system_.m_frontend, nullptr)}
  , m_buffer{Utility::exchange(particle_system_.m_buffer, nullptr)}
  , m_technique{Utility::exchange(particle_system_.m_technique, nullptr)}
  , m_last_id{Utility::exchange(particle_system_.m_last_id, 0)}
  , m_count{Utility::exchange(particle_system_.m_count, 0)}
{
}

inline ParticleSystem::~ParticleSystem() {
  release();
}

inline ParticleSystem& ParticleSystem::operator=(ParticleSystem&& particle_system_) {
  if (this != &particle_system_) {
    release();
    m_frontend = Utility::exchange(particle_system_.m_frontend, nullptr);
    m_buffer = Utility::exchange(particle_system_.m_buffer, nullptr);
    m_technique = Utility::exchange(particle_system_.m_technique, nullptr);
    m_last_id = Utility::exchange(particle_system_.m_last_id, 0);
    m_count = Utility::exchange(particle_system_.m_count, 0);
  }
  return *this;
}

} // namespace Rx::Render

#endif // RX_RENDER_PARTICLE_SYSTEM_H