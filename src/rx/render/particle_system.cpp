#include <stddef.h> // offsetof

#include "rx/render/particle_system.h"

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/target.h"

#include "rx/particle/system.h"

#include "rx/math/frustum.h"

namespace Rx::Render {

Optional<ParticleSystem> ParticleSystem::create(Frontend::Context* _frontend) {
  auto technique = _frontend->find_technique_by_name("particle_system");
  if (!technique) {
    return nullopt;
  }

  Frontend::Buffer::Format format{_frontend->allocator()};
  format.record_element_type(Frontend::Buffer::ElementType::NONE);
  format.record_vertex_stride(sizeof(Vertex));
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, position)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32, offsetof(Vertex, size)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::U8x4, offsetof(Vertex, color)});
  format.finalize();

  auto buffer = _frontend->create_buffer(RX_RENDER_TAG("ParticleSystem"));
  if (!buffer) {
    return nullopt;
  }

  if (!buffer->record_format(format)) {
    _frontend->destroy_buffer(RX_RENDER_TAG("ParticleSystem"), buffer);
    return nullopt;
  }

  buffer->record_type(Frontend::Buffer::Type::DYNAMIC);

  _frontend->initialize_buffer(RX_RENDER_TAG("ParticleSystem"), buffer);

  return ParticleSystem{_frontend, buffer, technique};
}

void ParticleSystem::release() {
  if (m_frontend) {
    m_frontend->destroy_buffer(RX_RENDER_TAG("ParticleSystem"), m_buffer);
  }
}

void ParticleSystem::render(const Particle::System* _system,
  Frontend::Target* _target, const Math::Mat4x4f& _model,
  const Math::Mat4x4f& _view,const Math::Mat4x4f& _projection)
{
  Math::Frustum frustum{_view * _projection};

  auto count = _system->alive_count();
  if (count == 0) {
    return;
  }

  // Only update the content if this is a new state.
  if (_system->id() != m_last_id) {
    if (!m_indices.resize(count)) {
      return;
    }

    m_count = _system->visible({m_indices.data(), m_indices.size()}, frustum);

    auto vertices = reinterpret_cast<Vertex*>(m_buffer->map_vertices(sizeof(Vertex) * m_count));
    if (!vertices) {
      return;
    }

    for (Size i = 0; i < m_count; i++) {
      const auto index = m_indices[i];
      vertices[i].size = _system->size(index);
      vertices[i].position = _system->position(index);
      vertices[i].color = _system->color(index);
    }

    m_buffer->record_vertices_edit(0, m_count * sizeof(Vertex));
    m_frontend->update_buffer(RX_RENDER_TAG("ParticleSysten"), m_buffer);

    m_last_id = _system->id();
  }

  // Nothing to render.
  if (m_count == 0) {
    return;
  }

  Frontend::State state;
  state.cull.record_enable(false);
  state.blend.record_enable(true);
  state.blend.record_blend_factors(
    Frontend::BlendState::FactorType::SRC_ALPHA,
    Frontend::BlendState::FactorType::ONE_MINUS_SRC_ALPHA);
  state.depth.record_test(true);
  state.depth.record_write(true);
  state.viewport.record_dimensions(_target->dimensions());

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Program* program = m_technique->configuration(0).basic();
  program->uniforms()[0].record_mat4x4f(_model * _view);
  program->uniforms()[1].record_mat4x4f(_projection);

  m_frontend->draw(
    RX_RENDER_TAG("ParticleSystem"),
    state,
    _target,
    draw_buffers,
    m_buffer,
    program,
    m_count,
    0,
    0,
    0,
    0,
    Frontend::PrimitiveType::POINTS,
    {});
}

} // namespace Rx::Render