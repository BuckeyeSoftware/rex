#include "rx/render/gbuffer.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render {

Optional<GBuffer> GBuffer::create(Frontend::Context* _frontend,
  const Math::Vec2z& _resolution)
{
  auto albedo = _frontend->create_texture2D(RX_RENDER_TAG("gbuffer albedo"));
  auto normal = _frontend->create_texture2D(RX_RENDER_TAG("gbuffer normal"));
  auto emission = _frontend->create_texture2D(RX_RENDER_TAG("gbuffer emission"));
  auto target = _frontend->create_target(RX_RENDER_TAG("gbuffer"));

  if (!albedo || !normal || !emission || !target) {
    _frontend->destroy_target(RX_RENDER_TAG("gbuffer"), target);
    _frontend->destroy_texture(RX_RENDER_TAG("gbuffer emission"), emission);
    _frontend->destroy_texture(RX_RENDER_TAG("gbuffer normal"), normal);
    _frontend->destroy_texture(RX_RENDER_TAG("gbuffer albedo"), albedo);
    return nullopt;
  }

  // Albedo.
  albedo->record_format(Frontend::Texture::DataFormat::SRGBA_U8);
  albedo->record_type(Frontend::Texture::Type::ATTACHMENT);
  albedo->record_levels(1);
  albedo->record_dimensions(_resolution);
  albedo->record_filter({false, false, false});
  albedo->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                 Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  _frontend->initialize_texture(RX_RENDER_TAG("gbuffer albedo"), albedo);
  target->attach_texture(albedo, 0);

  // Normal.
  normal->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  normal->record_type(Frontend::Texture::Type::ATTACHMENT);
  normal->record_levels(1);
  normal->record_dimensions(_resolution);
  normal->record_filter({false, false, false});
  normal->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                 Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  _frontend->initialize_texture(RX_RENDER_TAG("gbuffer normal"), normal);
  target->attach_texture(normal, 0);

  // Emission.
  emission->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  emission->record_type(Frontend::Texture::Type::ATTACHMENT);
  emission->record_levels(1);
  emission->record_dimensions(_resolution);
  emission->record_filter({false, false, false});
  emission->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                   Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  _frontend->initialize_texture(RX_RENDER_TAG("gbuffer emission"), emission);
  target->attach_texture(emission, 0);

  // Depth & Stencil.
  target->request_depth_stencil(Frontend::Texture::DataFormat::D24_S8, _resolution);
  _frontend->initialize_target(RX_RENDER_TAG("gbuffer"), target);

  return GBuffer{_frontend, target, albedo, normal, emission};
}

GBuffer& GBuffer::operator=(GBuffer&& gbuffer_) {
  if (this != &gbuffer_) {
    release();

    m_frontend = Utility::exchange(gbuffer_.m_frontend, nullptr);
    m_target = Utility::exchange(gbuffer_.m_target, nullptr);
    m_albedo_texture = Utility::exchange(gbuffer_.m_albedo_texture, nullptr);
    m_normal_texture = Utility::exchange(gbuffer_.m_normal_texture, nullptr);
    m_emission_texture = Utility::exchange(gbuffer_.m_emission_texture, nullptr);
  }

  return *this;
}

void GBuffer::release() {
  if (!m_frontend) {
    return;
  }

  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer normal"), m_normal_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer emission"), m_emission_texture);
  m_frontend->destroy_target(RX_RENDER_TAG("gbuffer"), m_target);
}

Frontend::Texture2D* GBuffer::depth_stencil() const {
  return m_target->depth_stencil();
}

} // namespace Rx::Render
