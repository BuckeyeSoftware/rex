#include <stddef.h> // offsetof

#include "rx/render/particle_system.h"

#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/target.h"

#include "rx/particle/system.h"

namespace Rx::Render {

ParticleSystem::ParticleSystem(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_buffer{nullptr}
  , m_technique{nullptr}
  , m_count{0}
{
  m_technique = _frontend->find_technique_by_name("particle_system");
}

ParticleSystem::~ParticleSystem() {
  m_frontend->destroy_buffer(RX_RENDER_TAG("ParticleSystem"), m_buffer);
}

bool ParticleSystem::update(const Particle::System* _system) {
  if (!m_buffer) {
    Frontend::Buffer::Format format;
    format.record_type(Frontend::Buffer::Type::DYNAMIC);
    format.record_element_type(Frontend::Buffer::ElementType::NONE);
    format.record_vertex_stride(sizeof(Vertex));
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32, offsetof(Vertex, size)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, position)});
    format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x4, offsetof(Vertex, color)});
    format.finalize();

    // Create a buffer.
    auto buffer = m_frontend->create_buffer(RX_RENDER_TAG("ParticleSystem"));
    if (!buffer) {
      return false;
    }

    buffer->record_format(format);
    m_frontend->initialize_buffer(RX_RENDER_TAG("ParticleSystem"), buffer);

    m_buffer = buffer;
  }

  m_count = _system->alive_count();
  if (m_count == 0) {
    return true;
  }

  auto vertices = reinterpret_cast<Vertex*>(m_buffer->map_vertices(sizeof(Vertex) * m_count));
  if (!vertices) {
    return false;
  }

  for (Size i = 0; i < m_count; i++) {
    vertices[i].size = _system->size(i);
    vertices[i].position = _system->position(i);
    vertices[i].color = _system->color(i);
  }

  m_buffer->record_vertices_edit(0, m_count * sizeof(Vertex));

  m_frontend->update_buffer(RX_RENDER_TAG("ParticleSysten"), m_buffer);

  return true;
}

void ParticleSystem::render(Frontend::Target* _target,
  const Math::Mat4x4f& _model, const Math::Mat4x4f& _view,
  const Math::Mat4x4f& _projection)
{
  if (m_count == 0) {
    return;
  }

  Frontend::State state;
  state.cull.record_enable(false);
  state.blend.record_enable(true);
  state.blend.record_blend_factors(Frontend::BlendState::FactorType::SRC_ALPHA,
    Frontend::BlendState::FactorType::ONE_MINUS_SRC_ALPHA);
  state.depth.record_test(true);
  state.depth.record_write(true);
  state.viewport.record_dimensions(_target->dimensions());

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Program* program = m_technique->basic();
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