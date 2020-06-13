#include "rx/render/gbuffer.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render {

gbuffer::gbuffer(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_target{nullptr}
  , m_albedo_texture{nullptr}
  , m_normal_texture{nullptr}
  , m_emission_texture{nullptr}
{
}

gbuffer::~gbuffer() {
  destroy();
}

void gbuffer::destroy() {
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer normal"), m_normal_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer emission"), m_emission_texture);
  m_frontend->destroy_target(RX_RENDER_TAG("gbuffer"), m_target);
}

void gbuffer::create(const Math::Vec2z& _resolution) {
  m_albedo_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer albedo"));
  m_albedo_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_albedo_texture->record_type(Frontend::Texture::Type::k_attachment);
  m_albedo_texture->record_levels(1);
  m_albedo_texture->record_dimensions(_resolution);
  m_albedo_texture->record_filter({false, false, false});
  m_albedo_texture->record_wrap({
                                        Frontend::Texture::WrapType::k_clamp_to_edge,
                                        Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo_texture);

  m_normal_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer normal"));
  m_normal_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_normal_texture->record_type(Frontend::Texture::Type::k_attachment);
  m_normal_texture->record_levels(1);
  m_normal_texture->record_dimensions(_resolution);
  m_normal_texture->record_filter({false, false, false});
  m_normal_texture->record_wrap({
                                        Frontend::Texture::WrapType::k_clamp_to_edge,
                                        Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer normal"), m_normal_texture);

  m_emission_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer emission"));
  m_emission_texture->record_format(Frontend::Texture::DataFormat::k_rgba_u8);
  m_emission_texture->record_type(Frontend::Texture::Type::k_attachment);
  m_emission_texture->record_levels(1);
  m_emission_texture->record_dimensions(_resolution);
  m_emission_texture->record_filter({false, false, false});
  m_emission_texture->record_wrap({
                                          Frontend::Texture::WrapType::k_clamp_to_edge,
                                          Frontend::Texture::WrapType::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer emission"), m_emission_texture);

  m_target = m_frontend->create_target(RX_RENDER_TAG("gbuffer"));
  m_target->request_depth_stencil(Frontend::Texture::DataFormat::k_d24_s8, _resolution);
  m_target->attach_texture(m_albedo_texture, 0);
  m_target->attach_texture(m_normal_texture, 0);
  m_target->attach_texture(m_emission_texture, 0);
  m_frontend->initialize_target(RX_RENDER_TAG("gbuffer"), m_target);
}

void gbuffer::resize(const Math::Vec2z& _resolution) {
  destroy();
  create(_resolution);
}

Frontend::Texture2D* gbuffer::depth_stencil() const {
  return m_target->depth_stencil();
}

} // namespace rx::render
