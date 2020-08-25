#include "rx/render/indirect_lighting_pass.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"

#include "rx/render/gbuffer.h"
#include "rx/render/image_based_lighting.h"

namespace Rx::Render {

IndirectLightingPass::IndirectLightingPass(Frontend::Context* _frontend,
                                           const GBuffer* _gbuffer,
                                           const ImageBasedLighting* _ibl)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("deferred_indirect")}
  , m_texture{nullptr}
  , m_target{nullptr}
  , m_gbuffer{_gbuffer}
  , m_ibl{_ibl}
{
}

void IndirectLightingPass::render(const Math::Camera& _camera) {
  Frontend::State state;
  state.viewport.record_dimensions(m_target->dimensions());
  state.cull.record_enable(false);

  Frontend::Program* program{*m_technique};

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  m_frontend->clear(
    RX_RENDER_TAG("indirect lighting pass"),
    state,
    m_target,
    draw_buffers,
    RX_RENDER_CLEAR_COLOR(0),
    Math::Vec4f{0.0f, 0.0f, 0.0f, 1.0f}.data());

  Frontend::Textures draw_textures;
  program->uniforms()[0].record_sampler(draw_textures.add(m_gbuffer->albedo()));
  program->uniforms()[1].record_sampler(draw_textures.add(m_gbuffer->normal()));
  program->uniforms()[2].record_sampler(draw_textures.add(m_gbuffer->depth_stencil()));
  program->uniforms()[3].record_sampler(draw_textures.add(m_ibl->irradiance()));
  program->uniforms()[4].record_sampler(draw_textures.add(m_ibl->prefilter()));
  program->uniforms()[5].record_sampler(draw_textures.add(m_ibl->scale_bias()));
  program->uniforms()[6].record_mat4x4f(Math::Mat4x4f::invert(_camera.view() * _camera.projection));
  program->uniforms()[7].record_vec3f(_camera.translate);

  m_frontend->draw(
    RX_RENDER_TAG("indirect lighting pass"),
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
    Render::Frontend::PrimitiveType::TRIANGLES,
    draw_textures);
}

void IndirectLightingPass::create(const Math::Vec2z& _dimensions) {
  m_texture = m_frontend->create_texture2D(RX_RENDER_TAG("indirect lighting pass"));
  m_texture->record_type(Frontend::Texture::Type::k_attachment);
  m_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_texture->record_filter({false, false, false});
  m_texture->record_levels(1);
  m_texture->record_dimensions(_dimensions);
  m_texture->record_wrap({
    Frontend::Texture::WrapType::k_clamp_to_edge,
    Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("indirect lighting pass"), m_texture);

  m_target = m_frontend->create_target(RX_RENDER_TAG("indirect lighting pass"));
  m_target->attach_texture(m_texture, 0);
  m_target->attach_depth_stencil(m_gbuffer->depth_stencil());
  m_frontend->initialize_target(RX_RENDER_TAG("indirect lighting pass"), m_target);
}

void IndirectLightingPass::destroy() {
  m_frontend->destroy_texture(RX_RENDER_TAG("indirect lighting pass"), m_texture);
  m_frontend->destroy_target(RX_RENDER_TAG("indirect lighting pass"), m_target);
}

} // namespace rx::render
