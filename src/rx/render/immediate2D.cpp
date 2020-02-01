#include <string.h> // strchr
#include <stdio.h> // sscanf
#include <inttypes.h> // PRIx32

#include "rx/render/immediate2D.h"

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"

#include "rx/math/constants.h"

#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/profiler.h"
#include "rx/core/json.h"

#include "rx/texture/chain.h"

#include "lib/stb_truetype.h"

namespace rx::render {

immediate2D::queue::queue(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_commands{m_allocator}
  , m_string_table{m_allocator}
{
}

immediate2D::queue::queue(queue&& queue_)
  : m_allocator{queue_.m_allocator}
  , m_commands{utility::move(queue_.m_commands)}
  , m_string_table{utility::move(queue_.m_string_table)}
{
  queue_.m_allocator = nullptr;
}

immediate2D::queue& immediate2D::queue::operator=(queue&& queue_) {
  RX_ASSERT(&queue_ != this, "self assignment");

  m_allocator = queue_.m_allocator;
  m_commands = utility::move(queue_.m_commands);
  m_string_table = utility::move(queue_.m_string_table);
  queue_.m_allocator = nullptr;

  return *this;
}

bool immediate2D::queue::command::operator!=(const command& _command) const {
  if (_command.hash != hash) {
    return true;
  }

  if (_command.kind != kind) {
    return true;
  }

  if (_command.flags != flags) {
    return true;
  }

  if (_command.color != color) {
    return true;
  }

  switch (kind) {
  case command::type::k_uninitialized:
    return false;
  case command::type::k_line:
    return _command.as_line != as_line;
  case command::type::k_rectangle:
    return _command.as_rectangle != as_rectangle;
  case command::type::k_scissor:
    return _command.as_scissor != as_scissor;
  case command::type::k_text:
    return _command.as_text != as_text;
  case command::type::k_triangle:
    return _command.as_triangle != as_triangle;
  }

  return false;
}

void immediate2D::queue::record_scissor(const math::vec2f& _position,
  const math::vec2f& _size)
{
  profiler::cpu_sample sample{"immediate2D::queue::record_scissor"};

  if (_position.x < 0.0f) {
    m_scissor = nullopt;
  } else {
    m_scissor = box{_position, _size};
  }

  command next_command;
  next_command.kind = command::type::k_scissor;
  next_command.flags = _position.x < 0.0f ? 0.0f : 1.0f;
  next_command.color = {};
  next_command.as_scissor.position = _position;
  next_command.as_scissor.size = _size;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2f>{}(next_command.as_scissor.position));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2f>{}(next_command.as_scissor.size));

  m_commands.push_back(utility::move(next_command));
}

void immediate2D::queue::record_rectangle(const math::vec2f& _position,
  const math::vec2f& _size, rx_f32 _roundness, const math::vec4f& _color)
{
  profiler::cpu_sample sample{"immediate2D::queue::record_rectangle"};

  command next_command;
  next_command.kind = command::type::k_rectangle;
  next_command.flags = 0;
  next_command.color = _color;
  next_command.as_rectangle.position = _position;
  next_command.as_rectangle.size = _size;
  next_command.as_rectangle.roundness = _roundness;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2f>{}(next_command.as_rectangle.position));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2f>{}(next_command.as_rectangle.size));
  next_command.hash = hash_combine(next_command.hash, hash<rx_f32>{}(next_command.as_rectangle.roundness));

  m_commands.push_back(utility::move(next_command));
}

void immediate2D::queue::record_line(const math::vec2f& _point_a,
  const math::vec2f& _point_b, rx_f32 _roundness, rx_f32 _thickness,
  const math::vec4f& _color)
{
  profiler::cpu_sample sample{"immediate2D::queue::record_line"};

  command next_command;
  next_command.kind = command::type::k_line;
  next_command.flags = 0;
  next_command.color = _color;
  next_command.as_line.points[0] = _point_a;
  next_command.as_line.points[1] = _point_b;
  next_command.as_line.roundness = _roundness;
  next_command.as_line.thickness = _thickness;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));

  m_commands.push_back(utility::move(next_command));
}

void immediate2D::queue::record_triangle(const math::vec2f& _position,
  const math::vec2f& _size, rx_u32 _flags, const math::vec4f& _color)
{
  profiler::cpu_sample sample{"immediate2D::queue::record_triangle"};

  command next_command;
  next_command.kind = command::type::k_triangle;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_triangle.position = _position;
  next_command.as_triangle.size = _size;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash<math::vec2f>{}(next_command.as_triangle.position);
  next_command.hash = hash<math::vec2f>{}(next_command.as_triangle.size);

  m_commands.push_back(utility::move(next_command));
}

void immediate2D::queue::record_text(const char* _font, rx_size _font_length,
  const math::vec2f& _position, rx_s32 _size, rx_f32 _scale, text_align _align,
  const char* _text, rx_size _text_length, const math::vec4f& _color)
{
  profiler::cpu_sample sample{"immediate2D::queue::record_text"};

  if (_text_length == 0) {
    return;
  }

  // Quick and dirty rejection of text outside the scissor.
  if (m_scissor) {
    // The text is above the scissor rectangle.
    if (_position.y > m_scissor->position.y + m_scissor->size.h) {
      return;
    }

    // The text is below the scissor rectangle.
    if (_position.y < m_scissor->position.y) {
      return;
    }

    // Text is outside the right edge of the scissor while not right aligned.
    if (_align != text_align::k_right
      && _position.x > m_scissor->position.x + m_scissor->size.w) {
      return;
    }
  }

  command next_command;
  next_command.kind = command::type::k_text;
  next_command.flags = static_cast<rx_u32>(_align);
  next_command.color = _color;
  next_command.as_text.position = _position;
  next_command.as_text.size = _size;
  next_command.as_text.scale = _scale;
  next_command.as_text.font_length = _font_length;
  next_command.as_text.text_length = _text_length;

  // insert strings into string table
  const auto font_index{m_string_table.size()};
  next_command.as_text.font_index = font_index;
  m_string_table.resize(font_index + _font_length + 1);
  memcpy(m_string_table.data() + font_index, _font, _font_length + 1);

  const auto text_index{m_string_table.size()};
  next_command.as_text.text_index = text_index;
  m_string_table.resize(text_index + _text_length + 1);
  memcpy(m_string_table.data() + text_index, _text, _text_length + 1);

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2f>{}(next_command.as_text.position));
  next_command.hash = hash_combine(next_command.hash, hash<rx_s32>{}(next_command.as_text.size));
  next_command.hash = hash_combine(next_command.hash, hash<rx_f32>{}(next_command.as_text.scale));
  next_command.hash = hash_combine(next_command.hash, hash<rx_size>{}(next_command.as_text.font_index));
  next_command.hash = hash_combine(next_command.hash, hash<rx_size>{}(next_command.as_text.font_length));
  next_command.hash = hash_combine(next_command.hash, hash<rx_size>{}(next_command.as_text.text_index));
  next_command.hash = hash_combine(next_command.hash, hash<rx_size>{}(next_command.as_text.text_length));

  m_commands.push_back(utility::move(next_command));
}

void immediate2D::queue::record_text(const char* _font,
  const math::vec2f& _position, rx_s32 _size, rx_f32 _scale, text_align _align,
  const char* _contents, const math::vec4f& _color)
{
  record_text(_font, strlen(_font), _position, _size, _scale, _align, _contents,
    strlen(_contents), _color);
}

bool immediate2D::queue::operator!=(const queue& _queue) const {
  if (_queue.m_commands.size() != m_commands.size()) {
    return true;
  }

  if (_queue.m_string_table.size() != m_string_table.size()) {
    return true;
  }

  for (rx_size i{0}; i < m_commands.size(); i++) {
    if (_queue.m_commands[i] != m_commands[i]) {
      return true;
    }
  }

  if (memcmp(_queue.m_string_table.data(), m_string_table.data(),
    m_string_table.size()) != 0)
  {
    return true;
  }

  return false;
}

void immediate2D::queue::clear() {
  m_commands.clear();
  m_string_table.clear();
}

immediate2D::font::font(const key& _key, frontend::interface* _frontend)
  : m_frontend{_frontend}
  , m_size{_key.size}
  , m_resolution{k_default_resolution}
  , m_texture{nullptr}
  , m_glyphs{m_frontend->allocator()}
{
  const auto data{filesystem::read_binary_file(string::format("base/fonts/%s.ttf", _key.name))};
  if (data) {
    static const int k_glyphs{96}; // all of ascii

    // figure out the atlas size needed
    for (;;) {
      vector<stbtt_bakedchar> baked_glyphs(m_frontend->allocator(), k_glyphs);
      vector<rx_byte> baked_atlas(m_frontend->allocator(), m_resolution*m_resolution);

      const int result{stbtt_BakeFontBitmap(data->data(), 0,
        static_cast<rx_f32>(m_size), baked_atlas.data(),
        static_cast<int>(m_resolution), static_cast<int>(m_resolution), 32,
        k_glyphs, baked_glyphs.data())};

      if (result == -k_glyphs || result > 0) {
        // create a texture chain from this baked font bitmap
        rx::texture::chain chain{m_frontend->allocator()};
        chain.generate(
          utility::move(baked_atlas),
          rx::texture::pixel_format::k_r_u8,
          rx::texture::pixel_format::k_r_u8,
          {m_resolution, m_resolution},
          false, true);

        // create and upload baked atlas
        m_texture = m_frontend->create_texture2D(RX_RENDER_TAG("font"));
        m_texture->record_format(frontend::texture::data_format::k_r_u8);
        m_texture->record_type(frontend::texture::type::k_static);
        m_texture->record_levels(chain.levels().size());
        m_texture->record_dimensions({m_resolution, m_resolution});
        m_texture->record_filter({true, false, true});
        m_texture->record_wrap({
          frontend::texture::wrap_type::k_clamp_to_edge,
          frontend::texture::wrap_type::k_clamp_to_edge});

        const auto& levels{chain.levels()};
        for (rx_size i{0}; i < levels.size(); i++) {
          const auto& level{levels[i]};
          m_texture->write(chain.data().data() + level.offset, i);
        }

        m_frontend->initialize_texture(RX_RENDER_TAG("font"), m_texture);

        // copy glyph information
        m_glyphs.resize(k_glyphs);
        for (int i{0}; i < k_glyphs; i++) {
          const auto& baked_glyph{baked_glyphs[i]};
          auto& glyph{m_glyphs[i]};

          glyph.x_advance   = baked_glyph.xadvance;
          glyph.offset      = {baked_glyph.xoff, baked_glyph.yoff};
          glyph.position[0] = {baked_glyph.x0,   baked_glyph.y0};
          glyph.position[1] = {baked_glyph.x1,   baked_glyph.y1};
        }

        break;
      }

      m_resolution *= 2;
    }
  }

  RX_ASSERT(m_texture, "could not create font texture");
}

immediate2D::font::font(font&& font_)
  : m_frontend{font_.m_frontend}
  , m_size{font_.m_size}
  , m_resolution{font_.m_resolution}
  , m_texture{font_.m_texture}
  , m_glyphs{utility::move(font_.m_glyphs)}
{
  font_.m_size = 0;
  font_.m_resolution = k_default_resolution;
  font_.m_texture = nullptr;
}

immediate2D::font::~font() {
  m_frontend->destroy_texture(RX_RENDER_TAG("font"), m_texture);
}

immediate2D::font::quad immediate2D::font::quad_for_glyph(rx_size _glyph,
  rx_f32 _scale, math::vec2f& position_) const
{
  const auto& glyph{m_glyphs[_glyph]};

  const math::vec2f scaled_offset{glyph.offset * _scale};
  const math::vec2f scaled_position[2]{
    glyph.position[0].cast<rx_f32>() * _scale,
    glyph.position[1].cast<rx_f32>() * _scale
  };

  const math::vec2f round{
    position_.x + scaled_offset.x,
    position_.y - scaled_offset.y
  };

  quad result;

  result.position[0] = round;
  result.position[1] = {
    round.x + scaled_position[1].x - scaled_position[0].x,
    round.y - scaled_position[1].y + scaled_position[0].y
  };

  result.coordinate[0] = glyph.position[0].cast<rx_f32>() / static_cast<rx_f32>(m_resolution);
  result.coordinate[1] = glyph.position[1].cast<rx_f32>() / static_cast<rx_f32>(m_resolution);

  position_.x += glyph.x_advance * _scale;

  return result;
}

immediate2D::immediate2D(frontend::interface* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("immediate2D")}
  , m_queue{m_frontend->allocator()}
  , m_vertices{m_frontend->allocator()}
  , m_elements{m_frontend->allocator()}
  , m_batches{m_frontend->allocator()}
  , m_vertex_index{0}
  , m_element_index{0}
  , m_rd_index{1}
  , m_wr_index{0}
{
  for (rx_size i{0}; i < k_buffers; i++) {
    m_render_batches[i] = {m_frontend->allocator()};
    m_render_queue[i] = {m_frontend->allocator()};
  }

  // generate circle geometry
  for (rx_size i{0}; i < k_circle_vertices; i++) {
    const rx_f32 phi{rx_f32(i) / rx_f32(k_circle_vertices) * math::k_pi<rx_f32> * 2.0f};
    m_circle_vertices[i] = {math::cos(phi), math::sin(phi)};
  }

  for (rx_size i{0}; i < k_buffers; i++) {
    m_buffers[i] = m_frontend->create_buffer(RX_RENDER_TAG("immediate2D"));
    m_buffers[i]->record_stride(sizeof(vertex));
    m_buffers[i]->record_type(frontend::buffer::type::k_dynamic);
    m_buffers[i]->record_element_type(frontend::buffer::element_type::k_u32);
    m_buffers[i]->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, position));
    m_buffers[i]->record_attribute(frontend::buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));
    m_buffers[i]->record_attribute(frontend::buffer::attribute::type::k_f32, 4, offsetof(vertex, color));
    m_frontend->initialize_buffer(RX_RENDER_TAG("immediate2D"), m_buffers[i]);
  }
}

immediate2D::~immediate2D() {
  for (rx_size i{0}; i < k_buffers; i++) {
    m_frontend->destroy_buffer(RX_RENDER_TAG("immediate2D"), m_buffers[i]);
  }

  m_fonts.each_value([this](font* _font) {
    m_frontend->allocator()->destroy<font>(_font);
  });
}

void immediate2D::immediate2D::render(frontend::target* _target) {
  profiler::cpu_sample sample{"immediate2D::render"};

  // avoid rendering if the last update did not produce any draw commands and
  // this iteration has no updates either
  const bool last_empty{m_render_queue[m_rd_index].is_empty()};
  if (last_empty && m_queue.is_empty()) {
    return;
  }

  // avoid generating geomtry and uploading if the contents didn't change
  if (m_queue != m_render_queue[m_rd_index]) {
    // calculate storage needed
    rx_size n_vertices{0};
    rx_size n_elements{0};
    m_queue.m_commands.each_fwd([&](const queue::command& _command) {
      switch (_command.kind) {
      case queue::command::type::k_rectangle:
        size_rectangle(
          _command.as_rectangle.roundness,
          n_vertices,
          n_elements);
          break;
      case queue::command::type::k_line:
        size_line(_command.as_line.roundness, n_vertices, n_elements);
        break;
      case queue::command::type::k_triangle:
        size_triangle(n_vertices, n_elements);
        break;
      case queue::command::type::k_text:
        size_text(
          m_queue.m_string_table.data() + _command.as_text.text_index,
          _command.as_text.text_length,
          n_vertices,
          n_elements);
        break;
      default:
        break;
      }
    });

    // allocate storage
    m_vertices.resize(n_vertices);
    m_elements.resize(n_elements);

    // generate geometry for a future frame
    m_queue.m_commands.each_fwd([this](const queue::command& _command) {
      switch (_command.kind) {
      case queue::command::type::k_rectangle:
        generate_rectangle(
          _command.as_rectangle.position.cast<rx_f32>(),
          _command.as_rectangle.size.cast<rx_f32>(),
          static_cast<rx_f32>(_command.as_rectangle.roundness),
          _command.color);
        break;
      case queue::command::type::k_line:
        generate_line(
          _command.as_line.points[0].cast<rx_f32>(),
          _command.as_line.points[1].cast<rx_f32>(),
          static_cast<rx_f32>(_command.as_line.thickness),
          static_cast<rx_f32>(_command.as_line.roundness),
          _command.color);
        break;
      case queue::command::type::k_triangle:
        generate_triangle(
          _command.as_triangle.position.cast<rx_f32>(),
          _command.as_triangle.size.cast<rx_f32>(),
          _command.color);
        break;
      case queue::command::type::k_text:
        generate_text(
          _command.as_text.size,
          m_queue.m_string_table.data() + _command.as_text.font_index,
          _command.as_text.font_length,
          m_queue.m_string_table.data() + _command.as_text.text_index,
          _command.as_text.text_length,
          _command.as_text.scale,
          _command.as_text.position.cast<rx_f32>(),
          static_cast<text_align>(_command.flags),
          _command.color);
        break;
      case queue::command::type::k_scissor:
        m_scissor_position = _command.as_scissor.position.cast<rx_s32>();
        m_scissor_size = _command.as_scissor.size.cast<rx_s32>();
        break;
      default:
        break;
      }
    });

    // upload the geometry
    m_buffers[m_wr_index]->write_vertices(m_vertices.data(), m_vertices.size() * sizeof(vertex));
    m_buffers[m_wr_index]->write_elements(m_elements.data(), m_elements.size() * sizeof(rx_u32));

    // record the edit
    m_buffers[m_wr_index]->record_vertices_edit(0, m_vertices.size() * sizeof(vertex));
    m_buffers[m_wr_index]->record_elements_edit(0, m_elements.size() * sizeof(rx_u32));
    m_frontend->update_buffer(RX_RENDER_TAG("immediate2D"), m_buffers[m_wr_index]);

    // clear staging buffers
    m_vertices.clear();
    m_elements.clear();

    m_vertex_index = 0;
    m_element_index = 0;

    // write buffer will be processed some time in the future
    m_render_batches[m_wr_index] = utility::move(m_batches);
    m_render_queue[m_wr_index] = utility::move(m_queue);

    m_wr_index = (m_wr_index + 1) % k_buffers;
  }

  // if the last queue has any draw commands, render them now
  const auto& dimensions{_target->dimensions().cast<rx_s32>()};
  m_technique->variant(0)->uniforms()[0].record_vec2i(dimensions);
  m_technique->variant(1)->uniforms()[0].record_vec2i(dimensions);

  if (!last_empty) {
    m_render_batches[m_rd_index].each_fwd([&](batch& _batch) {

      _batch.render_state.viewport.record_dimensions(_target->dimensions());

      frontend::buffers draw_buffers;
      draw_buffers.add(0);

      frontend::textures draw_textures;

      switch (_batch.kind) {
      case batch::type::k_triangles:
        m_frontend->draw(
          RX_RENDER_TAG("immediate2D triangles"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(0),
          _batch.count,
          _batch.offset,
          frontend::primitive_type::k_triangles,
          {});
          break;
      case batch::type::k_lines:
        m_frontend->draw(
          RX_RENDER_TAG("immediate2D lines"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(0),
          _batch.count,
          _batch.offset,
          frontend::primitive_type::k_lines,
          {});
          break;
      case batch::type::k_text:
        draw_textures.clear();
        draw_textures.add(_batch.texture);

        m_frontend->draw(
          RX_RENDER_TAG("immediate2D text"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(1),
          _batch.count,
          _batch.offset,
          frontend::primitive_type::k_triangles,
          draw_textures);
        break;
      default:
        break;
      }
    });

    m_rd_index = (m_rd_index + 1) % k_buffers;
  }

  m_queue.clear();
}

template<rx_size E>
void immediate2D::generate_polygon(const math::vec2f (&_coordinates)[E],
  const rx_f32 _thickness, const math::vec4f& _color)
{
  profiler::cpu_sample sample{"immediate2D::generate_polygon"};

  math::vec2f normals[E];
  math::vec2f coordinates[E];

  const rx_size offset{m_element_index};

  for (rx_size i{0}, j{E-1}; i < E; j = i++) {
    const math::vec2f& f0{coordinates[j]};
    const math::vec2f& f1{coordinates[i]};
    const math::vec2f delta{math::normalize(f1-f0)};
    normals[i] = {delta.y, -delta.x};
  }

  for (rx_size i{0}, j{E-1}; i < E; j = i++) {
    const math::vec2f& f0{normals[j]};
    const math::vec2f& f1{normals[i]};
    const math::vec2f normal{math::normalize((f0 + f1) * 0.5f)};
    coordinates[i] = _coordinates[i] + normal * _thickness;
  }

  // sanity check that we don't exceed the element format
  RX_ASSERT(m_vertex_index + E * 4 + (E -  2) * 3 <= 0xffffffff, "too many elements");

  for (rx_size i{0}, j{E-1}; i < E; j = i++) {
    const auto element{static_cast<rx_u32>(m_vertex_index)};

    add_element(element + 0);
    add_element(element + 1);
    add_element(element + 2);
    add_element(element + 2);
    add_element(element + 3);
    add_element(element + 0);

    add_vertex({_coordinates[i], {}, _color});
    add_vertex({_coordinates[j], {}, _color});
    add_vertex({coordinates[j], {}, {_color.r, _color.g, _color.b, 0.0f}});
    add_vertex({coordinates[i], {}, {_color.r, _color.g, _color.b, 0.0f}});
  }

  for (rx_size i{2}; i < E; i++) {
    const auto element{static_cast<rx_u32>(m_vertex_index)};

    add_element(element + 0);
    add_element(element + 1);
    add_element(element + 2);

    add_vertex({_coordinates[0], {}, _color});
    add_vertex({_coordinates[(i - 1)], {}, _color});
    add_vertex({_coordinates[i], {}, _color});
  }

  add_batch(offset, batch::type::k_triangles, _color.a < 1.0f);
}

void immediate2D::generate_rectangle(const math::vec2f& _position, const math::vec2f& _size,
  rx_f32 _roundness, const math::vec4f& _color)
{
  profiler::cpu_sample sample{"immediate2D::generate_rectangle"};

  if (_roundness > 0.0f) {
    static constexpr const rx_size k_round{k_circle_vertices/4};
    math::vec2f vertices[(k_round + 1) * 4];

    rx_size j{0};
    for (rx_size i{0}; i <= k_round; i++) {
      vertices[j++] = _position + _size - _roundness + m_circle_vertices[i] * _roundness;
    }

    for (rx_size i{k_round}; i <= k_round*2; i++) {
      vertices[j++] = _position + math::vec2f{_roundness, _size.h - _roundness} + m_circle_vertices[i] * _roundness;
    }

    for (rx_size i{k_round*2}; i <= k_round*3; i++) {
      vertices[j++] = _position + _roundness + m_circle_vertices[i] * _roundness;
    }

    for (rx_size i{k_round*3}; i < k_round*4; i++) {
      vertices[j++] = _position + math::vec2f{_size.w - _roundness, _roundness} + m_circle_vertices[i] * _roundness;
    }

    vertices[j++] = _position + math::vec2f{_size.w - _roundness, _roundness} + m_circle_vertices[0] * _roundness;

    generate_polygon(vertices, 1.0f, _color);
  } else {
    const math::vec2f vertices[]{
      {_position.x,           _position.y},
      {_position.x + _size.w, _position.y},
      {_position.x + _size.w, _position.y + _size.h},
      {_position.x,           _position.y + _size.h}
    };

    generate_polygon(vertices, 1.0f, _color);
  }
}

void immediate2D::generate_line(const math::vec2f& _point_a,
  const math::vec2f& _point_b, rx_f32 _thickness, rx_f32 _roundness,
  const math::vec4f& _color)
{
  profiler::cpu_sample smaple{"immediate2D::generate_line"};

  if (_roundness > 0.0f) {
    math::vec2f delta{math::normalize(_point_b - _point_a)};
    math::vec2f normal{delta.y, -delta.x};

    _roundness -= _thickness;
    _roundness *= 0.5f;

    delta = delta * math::vec2f{_roundness, _roundness};
    normal = normal * math::vec2f{_roundness, _roundness};

    const math::vec2f vertices[]{
      _point_a - delta - normal,
      _point_a - delta + normal,
      _point_b + delta + normal,
      _point_b + delta - normal
    };

    generate_polygon(vertices, _thickness, _color);
  } else {
    const rx_size offset{m_element_index};
    const auto element{static_cast<rx_u32>(m_vertex_index)};

    add_element(element + 0);
    add_element(element + 1);

    add_vertex({_point_a, {}, _color});
    add_vertex({_point_b, {}, _color});

    add_batch(offset, batch::type::k_lines, _color.a < 1.0f);
  }
}

static rx_size calculate_text_color(const char* _contents, math::vec4f& color_) {
  switch (*_contents) {
  case 'r':
    color_ = { 1.0f, 0.0f, 0.0f, 1.0f };
    return 1;
  case 'g':
    color_ = { 0.0f, 1.0f, 0.0f, 1.0f };
    return 1;
  case 'b':
    color_ = { 0.0f, 0.0f, 1.0f, 1.0f };
    return 1;
  case 'c':
    color_ = { 0.0f, 1.0f, 1.0f, 1.0f };
    return 1;
  case 'y':
    color_ = { 1.0f, 1.0f, 0.0f, 1.0f };
    return 1;
  case 'm':
    color_ = { 1.0f, 0.0f, 1.0f, 1.0f };
    return 1;
  case 'k':
    color_ = { 0.0f, 0.0f, 0.0f, 1.0f };
    return 1;
  case 'w':
    color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    return 1;
  }

  const char* terminate{strchr(_contents, ']')};
  if (*_contents == '[' && terminate) {
    rx_u32 color_hex{0};
    sscanf(_contents, "[%" PRIx32 "]", &color_hex);

    const auto a{static_cast<rx_f32>(color_hex & 0xff) / 255.0f};
    const auto b{static_cast<rx_f32>((color_hex >> 8) & 0xff) / 255.0f};
    const auto g{static_cast<rx_f32>((color_hex >> 16) & 0xff) / 255.0f};
    const auto r{static_cast<rx_f32>((color_hex >> 24) & 0xff) / 255.0f};

    color_ = { r, g, b, a };
    return terminate - _contents + 1;
  }

  return 0;
}

static rx_f32 calculate_text_length(immediate2D::font* _font, rx_f32 _scale,
  const char* _text, rx_size _text_length)
{
  rx_f32 position{0.0f};

  for (rx_size i{0}; i < _text_length; i++) {
    const int ch{_text[i]};
    if (ch == '^') {
      const char* next{_text + i + 1};
      if (*next != '^') {
        math::vec4f ignore;
        i += calculate_text_color(next, ignore);
        continue;
      }
    }

    const auto glyph{_font->glyph_for_code(ch - 32)};
    position += glyph.x_advance * _scale;
  }

  return position;
}

rx_f32 immediate2D::measure_text_length(const char* _font,
  const char* _text, rx_size _text_length, rx_s32 _size, rx_f32 _scale)
{
  profiler::cpu_sample sample{"immediate2D::measure_text_length"};

  const font::key key{_size, _font};
  const auto find{m_fonts.find(key)};
  font* font_map{nullptr};
  if (find) {
    font_map = *find;
  } else {
    font_map = m_frontend->allocator()->create<font>(key, m_frontend);
    m_fonts.insert(key, font_map);
  }

  return calculate_text_length(font_map, _scale, _text, _text_length);
}

void immediate2D::generate_text(rx_s32 _size, const char* _font,
  rx_size _font_length, const char* _contents, rx_size _contents_length,
  rx_f32 _scale, const math::vec2f& _position, text_align _align,
  const math::vec4f& _color)
{
  profiler::cpu_sample sample{"immediate2D::generate_text"};

  (void)_font_length;

  const font::key key{_size, _font};
  const auto find{m_fonts.find(key)};
  font* font_map{nullptr};
  if (find) {
    font_map = *find;
  } else {
    font_map = m_frontend->allocator()->create<font>(key, m_frontend);
    m_fonts.insert(key, font_map);
  }

  math::vec2f position{_position};
  math::vec4f color{_color};

  switch (_align) {
  case text_align::k_center:
    position.x -= calculate_text_length(font_map, _scale, _contents, _contents_length) * .5f;
    break;
  case text_align::k_right:
    position.x -= calculate_text_length(font_map, _scale, _contents, _contents_length);
    break;
  case text_align::k_left:
    break;
  }

  // scale relative to font size
  // _scale /= static_cast<rx_f32>(font_map->size());

  const rx_size offset{m_element_index};
  for (rx_size i{0}; i < _contents_length; i++) {
    const int ch{_contents[i]};
    if (ch == '^') {
      const char* next{_contents + i + 1};
      if (*next != '^') {
        i += calculate_text_color(next, color);
        continue;
      }
    }

    const font::quad quad{font_map->quad_for_glyph(ch - 32, _scale, position)};
    const auto element{static_cast<rx_u32>(m_vertex_index)};

    add_element(element + 0);
    add_element(element + 1);
    add_element(element + 2);
    add_element(element + 0);
    add_element(element + 3);
    add_element(element + 1);

    add_vertex({quad.position[0], quad.coordinate[0], color});
    add_vertex({quad.position[1], quad.coordinate[1], color});
    add_vertex({{quad.position[1].x, quad.position[0].y}, {quad.coordinate[1].s, quad.coordinate[0].t}, color});
    add_vertex({{quad.position[0].x, quad.position[1].y}, {quad.coordinate[0].s, quad.coordinate[1].t}, color});
  }

  add_batch(offset, batch::type::k_text, true, font_map->texture());
}

void immediate2D::generate_triangle(const math::vec2f& _position,
  const math::vec2f& _size, const math::vec4f& _color)
{
  const math::vec2f coordinates[]{
    _position,
    {_position.x + _size.w, _position.y + _size.h / 2.0f},
    {_position.x, _position.y + _size.h}
  };
  generate_polygon(coordinates, 1.0f, _color);
}

template<rx_size E>
void immediate2D::size_polygon(rx_size& n_vertices_, rx_size& n_elements_) {
  n_vertices_ += 4 * E + 3 * (E - 2);
  n_elements_ += 6 * E + 3 * (E - 2);
}

void immediate2D::size_rectangle(rx_f32 _roundness, rx_size& n_vertices_, rx_size& n_elements_) {
  if (_roundness > 0.0f) {
    static constexpr const rx_size k_round{k_circle_vertices/4};
    size_polygon<(k_round + 1) * 4>(n_vertices_, n_elements_);
  } else {
    size_polygon<4>(n_vertices_, n_elements_);
  }
}

void immediate2D::size_line(rx_f32 _roundness, rx_size& n_vertices_, rx_size& n_elements_) {
  if (_roundness > 0.0f) {
    size_polygon<4>(n_vertices_, n_elements_);
  } else {
    n_vertices_ += 2;
    n_elements_ += 2;
  }
}

void immediate2D::size_text(const char* _contents, rx_size _contents_length,
  rx_size& n_vertices_, rx_size& n_elements_)
{
  for (rx_size i{0}; i < _contents_length; i++) {
    const int ch{_contents[i]};
    if (ch == '^') {
      const char* next{_contents + i + 1};
      if (*next != '^') {
        math::vec4f sink;
        i += calculate_text_color(next, sink);
        continue;
      }
    }

    n_vertices_ += 4;
    n_elements_ += 6;
  }
}

void immediate2D::size_triangle(rx_size& n_vertices_, rx_size& n_elements_) {
  size_polygon<3>(n_vertices_, n_elements_);
}

void immediate2D::add_batch(rx_size _offset, batch::type _type, bool _blend,
  frontend::texture2D* _texture)
{
  profiler::cpu_sample sample{"immediate2D::add_batch"};

  const rx_size count{m_element_index - _offset};
  if (count == 0) {
    // Generated no geometry for this batch, discard it.
    return;
  }

  frontend::state render_state;

  if (_blend) {
    render_state.blend.record_enable(true);
    render_state.blend.record_blend_factors(
      frontend::blend_state::factor_type::k_src_alpha,
      frontend::blend_state::factor_type::k_one_minus_src_alpha);
  } else {
    render_state.blend.record_enable(false);
  }

  render_state.depth.record_test(false);
  render_state.depth.record_write(false);

  render_state.cull.record_enable(false);

  render_state.scissor.record_enable(m_scissor_size.w > 0);
  render_state.scissor.record_offset(m_scissor_position);
  render_state.scissor.record_size(m_scissor_size);

  render_state.flush();

  if (!m_batches.is_empty()) {
    auto& batch{m_batches.last()};
    if (batch.render_state == render_state && batch.kind == _type && batch.texture == _texture) {
      batch.count += count;
      return;
    }
  }

  m_batches.emplace_back(_offset, count, _type, render_state, _texture);
}

void immediate2D::add_element(rx_u32 _element) {
  m_elements[m_element_index++] = _element;
}

void immediate2D::add_vertex(vertex&& vertex_) {
  m_vertices[m_vertex_index++] = utility::move(vertex_);
}


} // namespace rx::render::frontend
