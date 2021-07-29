#include "rx/render/lens_distortion_pass.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/program.h"
#include "rx/render/frontend/state.h"

namespace Rx::Render {

Optional<LensDistortionPass> LensDistortionPass::create(
  Frontend::Context* _frontend, const Math::Vec2z& _resolution)
{
  auto technique = _frontend->find_technique_by_name("lens_distortion");
  if (!technique) {
    return nullopt;
  }

  auto texture = _frontend->create_texture2D(RX_RENDER_TAG("LensDistortionPass"));
  if (!texture) {
    return nullopt;
  }

  auto target = _frontend->create_target(RX_RENDER_TAG("LensDistortionPass"));
  if (!target) {
    _frontend->destroy_texture(RX_RENDER_TAG("LensDistortionPass"), texture);
    return nullopt;
  }

  texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  texture->record_levels(1);
  texture->record_dimensions(_resolution);
  _frontend->initialize_texture(RX_RENDER_TAG("LensDistortionPass"), texture);

  target->attach_texture(texture, 0);

  _frontend->initialize_target(RX_RENDER_TAG("LensDistortionPass"), target);

  return LensDistortionPass{_frontend, technique, texture, target};
}

void LensDistortionPass::release() {
  if (m_frontend) {
    m_frontend->destroy_target(RX_RENDER_TAG("LensDistortionPass"), m_target);
    m_frontend->destroy_texture(RX_RENDER_TAG("LensDistortionPass"), m_texture);
  }
}

void LensDistortionPass::render(Frontend::Texture2D* _source) {
  const auto& dimensions = m_texture->dimensions();

  Frontend::Program* program = m_technique->configuration(0).basic();

  program->uniforms()[0].record_vec3f({scale, dispersion, distortion});

  Frontend::Buffers draw_buffers;
  draw_buffers.add(0);

  Frontend::Sampler sampler;
  sampler.record_address_mode_u(Frontend::Sampler::AddressMode::CLAMP_TO_EDGE);
  sampler.record_address_mode_v(Frontend::Sampler::AddressMode::CLAMP_TO_EDGE);
  sampler.record_min_filter(Frontend::Sampler::Filter::NEAREST);
  sampler.record_mag_filter(Frontend::Sampler::Filter::NEAREST);
  sampler.record_mipmap_mode(Frontend::Sampler::MipmapMode::NONE);

  Frontend::Images draw_images;
  draw_images.add(_source, sampler);

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
    draw_images);
}

} // namespace Rx::Render
