#ifndef RX_RENDER_PARTICLE_SYSTEM_H
#define RX_RENDER_PARTICLE_SYSTEM_H
#include "rx/math/vec3.h"
#include "rx/math/mat4x4.h"

namespace Rx::Particle {
  struct System;
} // namespace Rx::Particle

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Buffer;
  struct Technique;
  struct Target;
};

struct ParticleSystem {
  ParticleSystem(Frontend::Context* _frontend);
  ~ParticleSystem();

  bool update(const Particle::System* _system);

  void render(Frontend::Target* _target,
    const Math::Mat4x4f& _model, const Math::Mat4x4f& _view,
    const Math::Mat4x4f& _projection);

private:
  struct Vertex {
    Float32 size;
    Math::Vec3f position;
    Math::Vec4f color;
  };

  Frontend::Context* m_frontend;
  Frontend::Buffer* m_buffer;
  Frontend::Technique* m_technique;
  Size m_count;
};

} // namespace Rx::Render

#endif // RX_RENDER_PARTICLE_SYSTEM_H