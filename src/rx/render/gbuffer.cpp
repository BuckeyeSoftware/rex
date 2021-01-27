#include "rx/render/gbuffer.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render {

GBuffer::GBuffer(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_target{nullptr}
  , m_albedo_texture{nullptr}
  , m_normal_texture{nullptr}
  , m_emission_texture{nullptr}
{
}

GBuffer::~GBuffer() {
  destroy();
}

void GBuffer::destroy() {
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer normal"), m_normal_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer emission"), m_emission_texture);
  m_frontend->destroy_target(RX_RENDER_TAG("gbuffer"), m_target);
}

void GBuffer::create(const Math::Vec2z& _resolution) {
  m_albedo_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer albedo"));
  m_albedo_texture->record_format(Frontend::Texture::DataFormat::SRGBA_U8);
  m_albedo_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_albedo_texture->record_levels(1);
  m_albedo_texture->record_dimensions(_resolution);
  m_albedo_texture->record_filter({false, false, false});
  m_albedo_texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                 Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo_texture);

  m_normal_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer normal"));
  m_normal_texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  m_normal_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_normal_texture->record_levels(1);
  m_normal_texture->record_dimensions(_resolution);
  m_normal_texture->record_filter({false, false, false});
  m_normal_texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                 Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer normal"), m_normal_texture);

  m_emission_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer emission"));
  m_emission_texture->record_format(Frontend::Texture::DataFormat::RGBA_U8);
  m_emission_texture->record_type(Frontend::Texture::Type::ATTACHMENT);
  m_emission_texture->record_levels(1);
  m_emission_texture->record_dimensions(_resolution);
  m_emission_texture->record_filter({false, false, false});
  m_emission_texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                   Frontend::Texture::WrapType::CLAMP_TO_EDGE});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer emission"), m_emission_texture);

  m_target = m_frontend->create_target(RX_RENDER_TAG("gbuffer"));
  m_target->attach_texture(m_albedo_texture, 0);
  m_target->attach_texture(m_normal_texture, 0);
  m_target->attach_texture(m_emission_texture, 0);
  m_target->request_depth_stencil(Frontend::Texture::DataFormat::D24_S8, _resolution);
  m_frontend->initialize_target(RX_RENDER_TAG("gbuffer"), m_target);
}

void GBuffer::resize(const Math::Vec2z& _resolution) {
  destroy();
  create(_resolution);
}

Frontend::Texture2D* GBuffer::depth_stencil() const {
  return m_target->depth_stencil();
}

} // namespace Rx::Render
