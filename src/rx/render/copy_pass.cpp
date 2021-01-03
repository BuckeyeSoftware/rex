#include "rx/render/copy_pass.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/state.h"

namespace Rx::Render {

CopyPass::CopyPass(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_target{nullptr}
  , m_texture{nullptr}
  , m_technique{m_frontend->find_technique_by_name("copy")}
  , m_depth_stencil{nullptr}
{
}

CopyPass::~CopyPass() {
  destroy();
}

void CopyPass::create(const Math::Vec2z& _dimensions) {
  m_texture = m_frontend->create_texture2D(RX_RENDER_TAG("CopyPass"));
  m_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_texture->record_filter({true, false, false});
  m_texture->record_levels(1);
  m_texture->record_dimensions(_dimensions);
  m_texture->record_wrap({
    Frontend::Texture::WrapType::k_clamp_to_edge,
    Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("CopyPass"), m_texture);

  m_target = m_frontend->create_target(RX_RENDER_TAG("CopyPass"));
  m_target->attach_texture(m_texture, 0);
  if (m_depth_stencil) {
    m_target->attach_depth_stencil(m_depth_stencil);
  }
  m_frontend->initialize_target(RX_RENDER_TAG("CopyPass"), m_target);
}

void CopyPass::destroy() {
  m_frontend->destroy_target(RX_RENDER_TAG("CopyPass"), m_target);
  m_frontend->destroy_texture(RX_RENDER_TAG("CopyPass"), m_texture);
}

void CopyPass::render(Frontend::Texture2D* _source) {
  const auto& dimensions = m_texture->dimensions();

  Frontend::Program* program = *m_technique;

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Textures draw_textures;
  program->uniforms()[0].record_sampler(draw_textures.add(_source));

  Frontend::State state;
  state.viewport.record_dimensions(dimensions);
  state.cull.record_enable(false);

  state.depth.record_test(false);
  state.depth.record_write(false);

  m_frontend->draw(
    RX_RENDER_TAG("CopyPass"),
    state,
    m_target,
    draw_buffers,
    nullptr,
    program,
    3,
    0,
    0,
    0,
    0,
    Frontend::PrimitiveType::TRIANGLES,
    draw_textures);
}

} // namespace Rx::Render
