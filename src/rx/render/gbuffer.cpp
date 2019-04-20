#include <rx/render/gbuffer.h>
#include <rx/render/target.h>
#include <rx/render/texture.h>
#include <rx/render/frontend.h>

namespace rx::render {

gbuffer::gbuffer(frontend* _frontend)
  : m_frontend{_frontend}
  , m_target{nullptr}
  , m_albedo_texture{nullptr}
  , m_normal_texture{nullptr}
  , m_emission_texture{nullptr}
{
}

gbuffer::~gbuffer() {
  destroy();
  // destroy();
}

void gbuffer::destroy() {
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer normal"), m_normal_texture);
  m_frontend->destroy_texture(RX_RENDER_TAG("gbuffer emission"), m_emission_texture);
  m_frontend->destroy_target(RX_RENDER_TAG("gbuffer"), m_target);
}

void gbuffer::create(const math::vec2z& _resolution) {
  m_albedo_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer albedo"));
  m_albedo_texture->record_format(texture::data_format::k_rgba_u8);
  m_albedo_texture->record_type(texture::type::k_attachment);
  m_albedo_texture->record_filter({false, false, false});
  m_albedo_texture->record_dimensions(_resolution);
  m_albedo_texture->record_wrap({
    texture::wrap_options::type::k_clamp_to_edge,
    texture::wrap_options::type::k_clamp_to_edge,
    texture::wrap_options::type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer albedo"), m_albedo_texture);

  m_normal_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer normal"));
  m_normal_texture->record_format(texture::data_format::k_rgba_f16);
  m_normal_texture->record_type(texture::type::k_attachment);
  m_normal_texture->record_filter({ false, false, false });
  m_normal_texture->record_dimensions(_resolution);
  m_normal_texture->record_wrap({
    texture::wrap_options::type::k_clamp_to_edge,
    texture::wrap_options::type::k_clamp_to_edge,
    texture::wrap_options::type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer normal"), m_normal_texture);

  m_emission_texture = m_frontend->create_texture2D(RX_RENDER_TAG("gbuffer emission"));
  m_emission_texture->record_format(texture::data_format::k_rgba_u8);
  m_emission_texture->record_type(texture::type::k_attachment);
  m_emission_texture->record_filter({false, false, false});
  m_emission_texture->record_dimensions(_resolution);
  m_emission_texture->record_wrap({
    texture::wrap_options::type::k_clamp_to_edge,
    texture::wrap_options::type::k_clamp_to_edge,
    texture::wrap_options::type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("gbuffer emission"), m_emission_texture);

  m_target = m_frontend->create_target(RX_RENDER_TAG("gbuffer"));
  m_target->request_depth_stencil(texture::data_format::k_d24_s8, _resolution);
  m_target->attach_texture(m_albedo_texture);
  m_target->attach_texture(m_normal_texture);
  m_target->attach_texture(m_emission_texture);
  m_frontend->initialize_target(RX_RENDER_TAG("gbuffer"), m_target);
}

void gbuffer::resize(const math::vec2z& _resolution) {
  destroy();
  create(_resolution);
}

} // namespace rx::render