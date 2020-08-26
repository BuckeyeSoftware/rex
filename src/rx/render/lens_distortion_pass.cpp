#include "rx/render/lens_distortion_pass.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/state.h"

namespace Rx::Render {

LensDistortionPass::LensDistortionPass(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("lens_distortion")}
  , m_texture{nullptr}
  , m_target{nullptr}
{
}

void LensDistortionPass::create(const Math::Vec2z& _resolution) {
  m_texture = m_frontend->create_texture2D(RX_RENDER_TAG("LensDistortionPass"));
  m_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_texture->record_filter({true, false, false});
  m_texture->record_levels(1);
  m_texture->record_dimensions(_resolution);
  m_texture->record_wrap({
    Frontend::Texture::WrapType::k_clamp_to_edge,
    Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("LensDistortionPass"), m_texture);

  m_target = m_frontend->create_target(RX_RENDER_TAG("LensDistortionPass"));
  m_target->attach_texture(m_texture, 0);
  m_frontend->initialize_target(RX_RENDER_TAG("LensDistortionPass"), m_target);
}

void LensDistortionPass::destroy() {
  m_frontend->destroy_target(RX_RENDER_TAG("LensDistortionPass"), m_target);
  m_frontend->destroy_texture(RX_RENDER_TAG("LensDistortionPass"), m_texture);
}

void LensDistortionPass::render(Frontend::Texture2D* _source) {
  const auto& dimensions = m_texture->dimensions();

  Frontend::Program* program = *m_technique;

  program->uniforms()[0].record_vec3f({scale, dispersion, distortion});

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Textures draw_textures;
  draw_textures.add(_source);

  Frontend::State state;
  state.viewport.record_dimensions(dimensions);
  state.cull.record_enable(false);

  m_frontend->draw(
    RX_RENDER_TAG("LensDistortionPass"),
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
