#include <stddef.h> // offsetof

#include "rx/render/irradiance_map.h"
#include "rx/render/skybox.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"

namespace rx::render {

irradiance_map::irradiance_map(frontend::interface* _frontend,
  const math::vec2z& _dimensions)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("irradiance_map")}
  , m_buffer{m_frontend->cached_buffer("quad")}
  , m_target{m_frontend->create_target(RX_RENDER_TAG("irradiance map"))}
  , m_texture{m_frontend->create_textureCM(RX_RENDER_TAG("irradiance map"))}
{
  m_texture->record_format(frontend::texture::data_format::k_rgba_u8);
  m_texture->record_type(frontend::texture::type::k_attachment);
  m_texture->record_levels(1);
  m_texture->record_dimensions(_dimensions);
  m_texture->record_filter({false, false, false});
  m_texture->record_wrap({
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("irradiance map"), m_texture);

  m_target->attach_texture(m_texture, 0);
  m_frontend->initialize_target(RX_RENDER_TAG("irradiance map"), m_target);

  if (!m_buffer) {
    static constexpr const struct vertex {
      math::vec2f position;
      math::vec2f coordinate;
    } k_quad_vertices[]{
      {{-1.0f,  1.0f}, {0.0f, 1.0f}},
      {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
      {{-1.0f, -1.0f}, {0.0f, 0.0f}},
      {{ 1.0f, -1.0f}, {1.0f, 0.0f}}
    };

    m_buffer = m_frontend->create_buffer(RX_RENDER_TAG("irradiance map"));
    m_buffer->record_type(frontend::buffer::type::k_static);
    m_buffer->record_element_type(frontend::buffer::element_type::k_none);
    m_buffer->record_stride(sizeof(vertex));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, position));
    m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));

    m_buffer->write_vertices(k_quad_vertices, sizeof k_quad_vertices);

    m_frontend->initialize_buffer(RX_RENDER_TAG("irradiance map"), m_buffer);
    m_frontend->cache_buffer(m_buffer, "quad");
  }
}

irradiance_map::~irradiance_map() {
  m_frontend->destroy_target(RX_RENDER_TAG("irradiance map"), m_target);
  m_frontend->destroy_texture(RX_RENDER_TAG("irradiance map"), m_texture);
  m_frontend->destroy_buffer(RX_RENDER_TAG("irradiance map"), m_buffer);
}

void irradiance_map::render(const skybox& _skybox) {
  frontend::state state;
  state.viewport.record_dimensions(m_target->dimensions());

  m_frontend->draw(
    RX_RENDER_TAG("irradiance map"),
    state,
    m_target,
    "012345",
    m_buffer,
    *m_technique,
    4,
    0,
    frontend::primitive_type::k_triangle_strip,
    "c",
    _skybox.cubemap());
}

} // namespace rx::render
