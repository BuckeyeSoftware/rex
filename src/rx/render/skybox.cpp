#include "rx/render/skybox.h"
#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/math/vec3.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/json.h"
#include "rx/core/profiler.h"

#include "rx/texture/loader.h"
#include "rx/texture/convert.h"

namespace rx::render {

static constexpr const math::vec3f k_skybox_vertices[]{
  { 1.0f, -1.0f, -1.0f},
  { 1.0f, -1.0f,  1.0f},
  { 1.0f,  1.0f,  1.0f},
  { 1.0f,  1.0f, -1.0f},
  {-1.0f, -1.0f,  1.0f},
  {-1.0f, -1.0f, -1.0f},
  {-1.0f,  1.0f, -1.0f},
  {-1.0f,  1.0f,  1.0f},
};

static constexpr const rx_u8 k_skybox_elements[]{
  0, 1, 2, 2, 3, 0, // right
  4, 5, 6, 6, 7, 4, // left
  6, 3, 2, 2, 7, 6, // up
  4, 1, 0, 0, 5, 4, // down
  1, 4, 7, 7, 2, 1, // front
  5, 0, 3, 3, 6, 5  // back
};

skybox::skybox(frontend::interface* _interface)
  : m_frontend{_interface}
  , m_technique{m_frontend->find_technique_by_name("skybox")}
  , m_texture{nullptr}
  , m_buffer{nullptr}
{
  m_buffer = m_frontend->create_buffer(RX_RENDER_TAG("skybox"));
  m_buffer->record_type(frontend::buffer::type::k_static);
  m_buffer->record_element_type(frontend::buffer::element_type::k_u8);
  m_buffer->record_attribute(frontend::buffer::attribute::type::k_f32, 3, 0);
  m_buffer->record_stride(sizeof(math::vec3f));
  m_buffer->write_vertices(k_skybox_vertices, sizeof k_skybox_vertices);
  m_buffer->write_elements(k_skybox_elements, sizeof k_skybox_elements);
  m_frontend->initialize_buffer(RX_RENDER_TAG("skybox"), m_buffer);
}

skybox::~skybox() {
  m_frontend->destroy_texture(RX_RENDER_TAG("skybox"), m_texture);
  m_frontend->destroy_buffer(RX_RENDER_TAG("skybox"), m_buffer);
}

void skybox::render(frontend::target* _target, const math::mat4x4f& _view,
  const math::mat4x4f& _projection)
{
  profiler::cpu_sample sample{"skybox::render"};

  if (!m_texture) {
    return;
  }

  // eliminate translation from the view matrix
  math::mat4x4f view{_view};
  view.w = {0.0f, 0.0f, 0.0f, 1.0f};

  frontend::program* program{*m_technique};

  program->uniforms()[0].record_mat4x4f(view * _projection);

  frontend::state state;
  state.depth.record_test(true);
  state.depth.record_write(true);

  state.blend.record_enable(false);

  state.viewport.record_dimensions(_target->dimensions());

  m_frontend->draw(
      RX_RENDER_TAG("skybox"),
      state,
      _target,
      "0",
      m_buffer,
      program,
      36,
      0,
      frontend::primitive_type::k_triangles,
      "c",
      m_texture);
}

bool skybox::load(const string& _file_name) {
  auto data{filesystem::read_text_file(_file_name)};
  if (!data) {
    return false;
  }

  const json description{data->disown()};
  if (!description) {
    // could not parse json
    return false;
  }

  if (!description.is_object()) {
    // expected object
    return false;
  }

  const auto& name{description["name"]};
  if (!name || !name.is_string()) {
    return false;
  }

  const auto& faces{description["faces"]};
  if (!faces || !faces.is_array_of(json::type::k_string) || faces.size() != 6) {
    return false;
  }

  m_name = name.as_string();

  m_frontend->destroy_texture(RX_RENDER_TAG("skybox"), m_texture);
  m_texture = m_frontend->create_textureCM(RX_RENDER_TAG("skybox"));
  m_texture->record_type(frontend::texture::type::k_static);
  m_texture->record_format(frontend::texture::data_format::k_rgb_u8);
  m_texture->record_levels(1);
  m_texture->record_filter({false, false, false});
  m_texture->record_wrap({
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge,
    frontend::texture::wrap_type::k_clamp_to_edge});

  math::vec2z dimensions;
  frontend::textureCM::face face{frontend::textureCM::face::k_right};
  bool result{faces.each([&](const json& file_name) {
    texture::loader texture{m_frontend->allocator()};
    if (!texture.load(file_name.as_string(), texture::pixel_format::k_rgb_u8)) {
      return false;
    }

    if (dimensions.is_all(0)) {
      dimensions = texture.dimensions();
      m_texture->record_dimensions(dimensions);
    } else if (dimensions != texture.dimensions()) {
      return false;
    }

    if (texture.format() != texture::pixel_format::k_rgb_u8) {
      // Convert everything to RGB8 if not already.
      const vector<rx_byte>& data{texture::convert(
        m_frontend->allocator(),
        texture.data().data(),
        texture.dimensions().area(),
        texture.format(),
        texture::pixel_format::k_rgb_u8)};
      m_texture->write(data.data(), face, 0);
    } else {
      m_texture->write(texture.data().data(), face, 0);
    }
    face = static_cast<frontend::textureCM::face>(static_cast<rx_size>(face) + 1);
    return true;
  })};

  if (result) {
    m_frontend->initialize_texture(RX_RENDER_TAG("skybox"), m_texture);
    return true;
  }

  return false;
}

} // namespace rx::render
