#include <stddef.h> // offsetof

#include "rx/render/ibl.h"
#include "rx/render/skybox.h"

#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"

namespace rx::render {

static frontend::buffer* quad_buffer(frontend::interface* _frontend) {
  frontend::buffer* buffer{_frontend->cached_buffer("quad")};
  if (buffer) {
    return buffer;
  }

  static constexpr const struct vertex {
    math::vec2f position;
    math::vec2f coordinate;
  } k_quad_vertices[]{
    {{-1.0f,  1.0f}, {0.0f, 1.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 0.0f}}
  };

  buffer = _frontend->create_buffer(RX_RENDER_TAG("quad"));
  buffer->record_type(frontend::buffer::type::k_static);
  buffer->record_element_type(frontend::buffer::element_type::k_none);
  buffer->record_stride(sizeof(vertex));
  buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, position));
  buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));
  buffer->write_vertices(k_quad_vertices, sizeof k_quad_vertices);

  _frontend->initialize_buffer(RX_RENDER_TAG("quad"), buffer);
  _frontend->cache_buffer(buffer, "quad");

  return buffer;
}

irradiance_map::irradiance_map(frontend::interface* _frontend, rx_size _resolution)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("irradiance_map")}
  , m_buffer{quad_buffer(m_frontend)}
  , m_target{m_frontend->create_target(RX_RENDER_TAG("irradiance map"))}
  , m_texture{m_frontend->create_textureCM(RX_RENDER_TAG("irradiance map"))}
{
  m_texture->record_format(frontend::texture::data_format::k_rgba_u8);
  m_texture->record_type(frontend::texture::type::k_attachment);
  m_texture->record_levels(1);
  m_texture->record_dimensions({_resolution, _resolution});
  m_texture->record_filter({false, false, false});
  m_texture->record_wrap({
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("irradiance map"), m_texture);

  m_target->attach_texture(m_texture, 0);
  m_frontend->initialize_target(RX_RENDER_TAG("irradiance map"), m_target);
}

irradiance_map::~irradiance_map() {
  m_frontend->destroy_target(RX_RENDER_TAG("irradiance map"), m_target);
  m_frontend->destroy_texture(RX_RENDER_TAG("irradiance map"), m_texture);
  m_frontend->destroy_buffer(RX_RENDER_TAG("irradiance map"), m_buffer);
}

void irradiance_map::render(frontend::textureCM* _environment_map) {
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
    _environment_map);
}

// prefilter_environment_map
prefilter_environment_map::prefilter_environment_map(
    frontend::interface* _frontend, const rx_size _resolution)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("prefilter_environment_map")}
  , m_buffer{quad_buffer(m_frontend)}
  , m_targets{m_frontend->allocator()}
{
  const rx_size levels{math::log2(_resolution) + 1};

  m_texture = m_frontend->create_textureCM(RX_RENDER_TAG("prefilter environment map"));
  m_texture->record_format(frontend::texture::data_format::k_rgba_u8);
  m_texture->record_type(frontend::texture::type::k_attachment);
  m_texture->record_levels(levels);
  m_texture->record_dimensions({_resolution, _resolution});
  m_texture->record_filter({false, false, false});
  m_texture->record_wrap({
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("irradiance map"), m_texture);

  m_targets.resize(levels);
  for (rx_size i{0}; i < levels; i++) {
    frontend::target* target{m_frontend->create_target(RX_RENDER_TAG("prefilter environment map"))};
    target->attach_texture(m_texture, i);
    m_frontend->initialize_target(RX_RENDER_TAG("irradiance map"), target);
    m_targets[i] = target;
  }
}

prefilter_environment_map::~prefilter_environment_map() {
  m_frontend->destroy_buffer(RX_RENDER_TAG("prefilter environment map"), m_buffer);
  m_frontend->destroy_texture(RX_RENDER_TAG("prefilter environment map"), m_texture);

  m_targets.each_fwd([this](frontend::target* _target) {
    m_frontend->destroy_target(RX_RENDER_TAG("prefilter environment map"), _target);
  });
}

void prefilter_environment_map::render(frontend::textureCM* _environment_map) {
  frontend::program* program{*m_technique};
  rx_size level{0};

  m_targets.each_fwd([&](frontend::target* _target) {
    frontend::state state;
    state.viewport.record_dimensions(_target->dimensions());

    rx_f32 roughness{static_cast<rx_f32>(level) / (m_targets.size() - 1)};
    program->uniforms()[1].record_float(roughness);

    m_frontend->draw(
      RX_RENDER_TAG("prefilter environment map"),
      state,
      _target,
      "012345",
      m_buffer,
      program,
      4,
      0,
      frontend::primitive_type::k_triangle_strip,
      "c",
      _environment_map);

    level++;
  });
}

} // namespace rx::render
