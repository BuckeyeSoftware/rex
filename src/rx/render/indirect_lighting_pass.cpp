#include "rx/render/indirect_lighting_pass.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"

#include "rx/render/gbuffer.h"
#include "rx/render/image_based_lighting.h"

namespace Rx::Render {

Optional<IndirectLightingPass> IndirectLightingPass::create(
  Frontend::Context* _frontend, const Options& _options)
{
  auto technique = _frontend->find_technique_by_name("deferred_indirect");
  if (!technique) {
    return nullopt;
  }

  auto texture = _frontend->create_texture2D(RX_RENDER_TAG("indirect lighting pass"));
  if (!texture) {
    return nullopt;
  }

  texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  texture->record_levels(1);
  texture->record_dimensions(_options.dimensions);
  _frontend->initialize_texture(RX_RENDER_TAG("indirect lighting pass"), texture);

  auto target = _frontend->create_target(RX_RENDER_TAG("indirect lighting pass"));
  if (!target) {
    _frontend->destroy_texture(RX_RENDER_TAG("indirect lighting pass"), texture);
    return nullopt;
  }

  target->attach_texture(texture, 0);
  target->attach_depth_stencil(_options.stencil);
  _frontend->initialize_target(RX_RENDER_TAG("indirect lighting pass"), target);

  IndirectLightingPass indirect_lighting_pass;
  indirect_lighting_pass.m_frontend = _frontend;
  indirect_lighting_pass.m_target = target;
  indirect_lighting_pass.m_technique = technique;
  indirect_lighting_pass.m_texture = texture;

  return indirect_lighting_pass;
}

void IndirectLightingPass::render(const Math::Camera& _camera, const Input& _input) {
  Frontend::State state;
  state.viewport.record_dimensions(m_target->dimensions());
  state.cull.record_enable(false);

  Frontend::Program* program = m_technique->configuration(0).basic();

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  m_frontend->clear(
    RX_RENDER_TAG("indirect lighting pass"),
    state,
    m_target,
    draw_buffers,
    RX_RENDER_CLEAR_COLOR(0),
    Math::Vec4f{0.0f, 0.0f, 0.0f, 0.0f}.data());

  // Only where stencil = 1
  state.stencil.record_enable(true);
  state.stencil.record_function(Frontend::StencilState::FunctionType::EQUAL);
  state.stencil.record_reference(1);

  // LINEAR REPEAT for irradiance.
  Frontend::Sampler irradiance_sampler;
  irradiance_sampler.record_address_mode_u(Frontend::Sampler::AddressMode::REPEAT);
  irradiance_sampler.record_address_mode_v(Frontend::Sampler::AddressMode::REPEAT);
  irradiance_sampler.record_address_mode_w(Frontend::Sampler::AddressMode::REPEAT);
  irradiance_sampler.record_min_filter(Frontend::Sampler::Filter::LINEAR);
  irradiance_sampler.record_mag_filter(Frontend::Sampler::Filter::LINEAR);

  // LINEAR REPEAT MIPMAP LINEAR for prefilter
  Frontend::Sampler prefilter_sampler;
  prefilter_sampler.record_address_mode_u(Frontend::Sampler::AddressMode::REPEAT);
  prefilter_sampler.record_address_mode_v(Frontend::Sampler::AddressMode::REPEAT);
  prefilter_sampler.record_address_mode_w(Frontend::Sampler::AddressMode::REPEAT);
  prefilter_sampler.record_min_filter(Frontend::Sampler::Filter::LINEAR);
  prefilter_sampler.record_mag_filter(Frontend::Sampler::Filter::LINEAR);
  prefilter_sampler.record_mipmap_mode(Frontend::Sampler::MipmapMode::LINEAR);

  // LINEAR, CLAMP_TO_EDGE for scale bias
  Frontend::Sampler scale_bias_sampler;
  scale_bias_sampler.record_address_mode_u(Frontend::Sampler::AddressMode::CLAMP_TO_EDGE);
  scale_bias_sampler.record_address_mode_v(Frontend::Sampler::AddressMode::CLAMP_TO_EDGE);
  scale_bias_sampler.record_min_filter(Frontend::Sampler::Filter::LINEAR);
  scale_bias_sampler.record_mag_filter(Frontend::Sampler::Filter::LINEAR);

  Frontend::Sampler nearest;
  nearest.record_address_mode_u(Frontend::Sampler::AddressMode::REPEAT);
  nearest.record_address_mode_v(Frontend::Sampler::AddressMode::REPEAT);
  nearest.record_min_filter(Frontend::Sampler::Filter::NEAREST);
  nearest.record_mag_filter(Frontend::Sampler::Filter::NEAREST);
  nearest.record_mipmap_mode(Frontend::Sampler::MipmapMode::NONE);

  Frontend::Images draw_images;
  program->uniforms()[0].record_sampler(draw_images.add(_input.albedo, nearest));
  program->uniforms()[1].record_sampler(draw_images.add(_input.normal, nearest));
  program->uniforms()[2].record_sampler(draw_images.add(_input.emission, nearest));
  program->uniforms()[3].record_sampler(draw_images.add(_input.depth, nearest));
  program->uniforms()[4].record_sampler(draw_images.add(_input.irradiance, irradiance_sampler));
  program->uniforms()[5].record_sampler(draw_images.add(_input.prefilter, prefilter_sampler));
  program->uniforms()[6].record_sampler(draw_images.add(_input.scale_bias, scale_bias_sampler));
  program->uniforms()[7].record_mat4x4f(Math::invert(_camera.view() * _camera.projection));
  program->uniforms()[8].record_vec3f(_camera.translate);
  program->uniforms()[9].record_vec2f(m_target->dimensions().cast<Float32>());

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
    draw_images);
}

bool IndirectLightingPass::recreate(const Options& _options) {
  if (auto result = create(m_frontend, _options)) {
    *this = Utility::move(*result);
    return true;
  }
  return false;
}

void IndirectLightingPass::release() {
  if (m_frontend) {
    m_frontend->destroy_texture(RX_RENDER_TAG("indirect lighting pass"), m_texture);
    m_frontend->destroy_target(RX_RENDER_TAG("indirect lighting pass"), m_target);
  }
}

} // namespace Rx::Render
