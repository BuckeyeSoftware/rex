#include "rx/render/indirect_lighting_pass.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"

#include "rx/render/gbuffer.h"
#include "rx/render/ibl.h"

namespace rx::render {

indirect_lighting_pass::indirect_lighting_pass(frontend::context* _frontend,
  const gbuffer* _gbuffer, const ibl* _ibl)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("deferred_indirect")}
  , m_texture{nullptr}
  , m_target{nullptr}
  , m_gbuffer{_gbuffer}
  , m_ibl{_ibl}
{
}

indirect_lighting_pass::~indirect_lighting_pass() {
  destroy();
}

void indirect_lighting_pass::render(const math::camera& _camera) {
  frontend::state state;
  state.viewport.record_dimensions(m_target->dimensions());
  state.cull.record_enable(false);

  frontend::program* program{*m_technique};

  program->uniforms()[6].record_mat4x4f(math::mat4x4f::invert(_camera.view() * _camera.projection));
  program->uniforms()[7].record_vec3f(_camera.translate);


  frontend::buffers draw_buffers;
  draw_buffers.add(0);

  m_frontend->clear(
    RX_RENDER_TAG("indirect lighting pass"),
    state,
    m_target,
    draw_buffers,
    RX_RENDER_CLEAR_COLOR(0),
    math::vec4f{0.0f, 0.0f, 0.0f, 1.0f}.data());

  state.stencil.record_enable(true);

  // StencilFunc(GL_EQUAL, 1, 0xFF)
  state.stencil.record_function(render::frontend::stencil_state::function_type::k_equal);
  state.stencil.record_reference(1);
  state.stencil.record_mask(0xFF);

  frontend::textures draw_textures;
  draw_textures.add(m_gbuffer->albedo());
  draw_textures.add(m_gbuffer->normal());
  draw_textures.add(m_gbuffer->depth_stencil());
  draw_textures.add(m_ibl->irradiance());
  draw_textures.add(m_ibl->prefilter());
  draw_textures.add(m_ibl->scale_bias());

  m_frontend->draw(
    RX_RENDER_TAG("indirect lighting pass"),
    state,
    m_target,
    draw_buffers,
    nullptr,
    program,
    3,
    0,
    render::frontend::primitive_type::k_triangles,
    draw_textures);
}

void indirect_lighting_pass::resize(const math::vec2z& _dimensions) {
  destroy();
  create(_dimensions);
}

void indirect_lighting_pass::create(const math::vec2z& _dimensions) {
  m_texture = m_frontend->create_texture2D(RX_RENDER_TAG("indirect lighting pass"));
  m_texture->record_type(frontend::texture::type::k_attachment);
  m_texture->record_format(frontend::texture::data_format::k_rgb_u8);
  m_texture->record_filter({false, false, false});
  m_texture->record_levels(1);
  m_texture->record_dimensions(_dimensions);
  m_texture->record_wrap({
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge});
  m_frontend->initialize_texture(RX_RENDER_TAG("indirect lighting pass"), m_texture);

  m_target = m_frontend->create_target(RX_RENDER_TAG("indirect lighting pass"));
  m_target->attach_texture(m_texture, 0);
  m_target->attach_depth_stencil(m_gbuffer->depth_stencil());
  m_frontend->initialize_target(RX_RENDER_TAG("indirect lighting pass"), m_target);
}

void indirect_lighting_pass::destroy() {
  m_frontend->destroy_texture(RX_RENDER_TAG("indirect lighting pass"), m_texture);
  m_frontend->destroy_target(RX_RENDER_TAG("indirect lighting pass"), m_target);
}

} // namespace rx::render
