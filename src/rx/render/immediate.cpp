#include <stdlib.h> // abort
#include <string.h> // strchr
#include <stdio.h> // sscanf
#include <inttypes.h> // PRIx32

#include <rx/render/immediate.h>
#include <rx/render/frontend.h>
#include <rx/render/technique.h>

#include <rx/math/constants.h>
#include <rx/math/trig.h>

#include <rx/core/filesystem/file.h>
#include <rx/core/json.h>

#include <rx/texture/chain.h>

#include "lib/stb_truetype.h"

namespace rx::render {

immediate::queue::queue(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_commands{m_allocator}
  , m_string_table{m_allocator}
{
}

immediate::queue::queue(queue&& _queue)
  : m_allocator{_queue.m_allocator}
  , m_commands{utility::move(_queue.m_commands)}
  , m_string_table{utility::move(_queue.m_string_table)}
{
  _queue.m_allocator = nullptr;
}

immediate::queue& immediate::queue::operator=(queue&& _queue) {
  RX_ASSERT(this != &_queue, "move from self");
  m_allocator = _queue.m_allocator;
  m_commands = utility::move(_queue.m_commands);
  m_string_table = utility::move(_queue.m_string_table);
  _queue.m_allocator = nullptr;
  return *this;
}

bool immediate::queue::command::operator!=(const command& _command) const {
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

void immediate::queue::record_scissor(const math::vec2i& _position,
  const math::vec2i& _size)
{
  command next_command;
  next_command.kind = command::type::k_scissor;
  next_command.flags = _position.x < 0 ? 0 : 1;
  next_command.color = {};
  next_command.as_scissor.position = _position;
  next_command.as_scissor.size = _size;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2i>{}(next_command.as_scissor.position));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2i>{}(next_command.as_scissor.size));

  m_commands.push_back(utility::move(next_command));
}

void immediate::queue::record_rectangle(const math::vec2i& _position,
  const math::vec2i& _size, rx_s32 _roundness, const math::vec4f& _color)
{
  command next_command;
  next_command.kind = command::type::k_rectangle;
  next_command.flags = 0;
  next_command.color = _color;
  next_command.as_rectangle.position = _position;
  next_command.as_rectangle.size = _size;
  next_command.as_rectangle.roundness = _roundness;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2i>{}(next_command.as_rectangle.position));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2i>{}(next_command.as_rectangle.size));
  next_command.hash = hash_combine(next_command.hash, hash<rx_s32>{}(next_command.as_rectangle.roundness));

  m_commands.push_back(utility::move(next_command));
}

void immediate::queue::record_line(const math::vec2i& _point_a,
  const math::vec2i& _point_b, rx_s32 _roundness, rx_s32 _thickness,
  const math::vec4f& _color)
{
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

void immediate::queue::record_triangle(const math::vec2i& _position,
  const math::vec2i& _size, rx_u32 _flags, const math::vec4f& _color)
{
  command next_command;
  next_command.kind = command::type::k_triangle;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_triangle.position = _position;
  next_command.as_triangle.size = _size;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash<math::vec2i>{}(next_command.as_triangle.position);
  next_command.hash = hash<math::vec2i>{}(next_command.as_triangle.size);

  m_commands.push_back(utility::move(next_command));
}

void immediate::queue::record_text(const string& _font,
  const math::vec2i& _position, rx_s32 _size, rx_f32 _scale, text_align _align,
  const string& _text, const math::vec4f& _color)
{
  command next_command;
  next_command.kind = command::type::k_text;
  next_command.flags = static_cast<rx_u32>(_align);
  next_command.color = _color;
  next_command.as_text.position = _position;
  next_command.as_text.size = _size;
  next_command.as_text.scale = _scale;

  // insert strings into string table
  const auto font_index{m_string_table.size()};
  next_command.as_text.font_index = font_index;
  m_string_table.resize(font_index + _font.size() + 1);
  memcpy(m_string_table.data() + font_index, _font.data(), _font.size() + 1);

  const auto text_index{m_string_table.size()};
  next_command.as_text.text_index = text_index;
  m_string_table.resize(text_index + _text.size() + 1);
  memcpy(m_string_table.data() + text_index, _text.data(), _text.size() + 1);

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2i>{}(next_command.as_text.position));
  next_command.hash = hash_combine(next_command.hash, hash<rx_s32>{}(next_command.as_text.size));
  next_command.hash = hash_combine(next_command.hash, hash<rx_f32>{}(next_command.as_text.scale));
  next_command.hash = hash_combine(next_command.hash, hash<rx_size>{}(next_command.as_text.font_index));
  next_command.hash = hash_combine(next_command.hash, hash<rx_size>{}(next_command.as_text.text_index));

  m_commands.push_back(utility::move(next_command));
}

bool immediate::queue::operator!=(const queue& _queue) const {
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

void immediate::queue::clear() {
  m_commands.clear();
  m_string_table.clear();
}

immediate::font::font(const key& _key, frontend* _frontend)
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
      array<stbtt_bakedchar> baked_glyphs(m_frontend->allocator(), k_glyphs);
      array<rx_byte> baked_atlas(m_frontend->allocator(), m_resolution*m_resolution);

      const int result{stbtt_BakeFontBitmap(data->data(), 0,
        static_cast<rx_f32>(m_size), baked_atlas.data(),
        static_cast<int>(m_resolution), static_cast<int>(m_resolution), 32,
        k_glyphs, baked_glyphs.data())};
      
      if (result == -k_glyphs || result > 0) {
        // create a texture chain from this baked font bitmap
        rx::texture::chain chain{utility::move(baked_atlas),
          rx::texture::chain::pixel_format::k_r_u8, {m_resolution}, false, true};

        // create and upload baked atlas
        m_texture = m_frontend->create_texture2D(RX_RENDER_TAG("font"));
        m_texture->record_format(texture::data_format::k_r_u8);
        m_texture->record_type(texture::type::k_static);
        m_texture->record_filter({true, false, true});
        m_texture->record_dimensions({m_resolution, m_resolution});
        m_texture->record_wrap({
          texture::wrap_type::k_clamp_to_edge,
          texture::wrap_type::k_clamp_to_edge});
        
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

immediate::font::font(font&& _font)
  : m_frontend{_font.m_frontend}
  , m_size{_font.m_size}
  , m_resolution{_font.m_resolution}
  , m_texture{_font.m_texture}
  , m_glyphs{utility::move(_font.m_glyphs)}
{
  _font.m_size = 0;
  _font.m_resolution = k_default_resolution;
  _font.m_texture = nullptr;
}

immediate::font::~font() {
  m_frontend->destroy_texture(RX_RENDER_TAG("font"), m_texture);
}
  
immediate::font::quad immediate::font::quad_for_glyph(rx_size _glyph,
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

immediate::immediate(frontend* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("immediate")}
  , m_queue{m_frontend->allocator()}
  , m_vertices{m_frontend->allocator()}
  , m_elements{m_frontend->allocator()}
  , m_batches{m_frontend->allocator()}
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
    m_buffers[i] = m_frontend->create_buffer(RX_RENDER_TAG("immediate"));
    m_buffers[i]->record_stride(sizeof(vertex));
    m_buffers[i]->record_type(buffer::type::k_dynamic);
    m_buffers[i]->record_element_type(buffer::element_type::k_u32);
    m_buffers[i]->record_attribute(buffer::attribute::type::k_f32, 2, offsetof(vertex, position));
    m_buffers[i]->record_attribute(buffer::attribute::type::k_f32, 2, offsetof(vertex, coordinate));
    m_buffers[i]->record_attribute(buffer::attribute::type::k_f32, 4, offsetof(vertex, color));
    m_frontend->initialize_buffer(RX_RENDER_TAG("immediate"), m_buffers[i]);
  }
}

immediate::~immediate() {
  for (rx_size i{0}; i < k_buffers; i++) {
    m_frontend->destroy_buffer(RX_RENDER_TAG("immediate"), m_buffers[i]);
  }

  m_fonts.each([&](rx_size, const font::key&, font* _font) {
    utility::destruct_and_deallocate<font>(m_frontend->allocator(), _font);
  });
}

void immediate::immediate::render(target* _target) {
  // avoid rendering if the last update did not produce any draw commands and
  // this iteration has no updates either
  const bool last_empty{m_render_queue[m_rd_index].is_empty()};
  if (last_empty && m_queue.is_empty()) {
    return;
  }

  // avoid generating geomtry and uploading if the contents didn't change
  if (m_queue != m_render_queue[m_rd_index]) {
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
        // TODO(dweiler): implement
        break;
      case queue::command::type::k_text:
        generate_text(
          _command.as_text.size,
          m_queue.m_string_table.data() + _command.as_text.font_index,
          m_queue.m_string_table.data() + _command.as_text.text_index,
          _command.as_text.scale,
          _command.as_text.position.cast<rx_f32>(),
          static_cast<text_align>(_command.flags),
          _command.color);
        break;
      case queue::command::type::k_scissor:
        m_scissor_position = _command.as_scissor.position;
        m_scissor_size = _command.as_scissor.size;
        break;
      default:
        break;
      }
    });

    // upload the geometry
    m_buffers[m_wr_index]->flush();
    m_buffers[m_wr_index]->write_vertices(m_vertices.data(), m_vertices.size() * sizeof(vertex));
    m_buffers[m_wr_index]->write_elements(m_elements.data(), m_elements.size() * sizeof(rx_u32));

    m_frontend->update_buffer(RX_RENDER_TAG("immediate"), m_buffers[m_wr_index]);

    // clear staging buffers
    m_vertices.clear();
    m_elements.clear();

    // write buffer will be processed some time in the future
    m_render_batches[m_wr_index] = utility::move(m_batches);
    m_render_queue[m_wr_index] = utility::move(m_queue);

    m_wr_index = (m_wr_index + 1) % k_buffers;
  }

  // if the last queue has any draw commands, render them now
  m_technique->variant(0)->uniforms()[0].record_vec2i({1600, 900});
  m_technique->variant(1)->uniforms()[0].record_vec2i({1600, 900});

  if (!last_empty) {
    m_render_batches[m_rd_index].each_fwd([&](const batch& _batch) {
      switch (_batch.kind) {
      case queue::command::type::k_rectangle:
        [[fallthrough]];
      case queue::command::type::k_line:
        [[fallthrough]];
      case queue::command::type::k_triangle:
        m_frontend->draw_elements(
          RX_RENDER_TAG("immediate untextured"),
          _batch.render_state,
          _target,
          m_buffers[m_rd_index],
          m_technique->variant(0),
          _batch.count,
          _batch.offset,
          primitive_type::k_triangles,
          "");
          break;
      case queue::command::type::k_text:
        m_frontend->draw_elements(
          RX_RENDER_TAG("immediate textures"),
          _batch.render_state,
          _target,
          m_buffers[m_rd_index],
          m_technique->variant(1),
          _batch.count,
          _batch.offset,
          primitive_type::k_triangles,
          "2",
          _batch.texture);
          break;
      default:
        break;
        // TODO
      }
    });

    m_rd_index = (m_rd_index + 1) % k_buffers;
  }

  m_queue.clear();
}

template<rx_size E>
void immediate::generate_polygon(const math::vec2f (&_coordinates)[E],
  const rx_f32 _thickness, const math::vec4f& _color, queue::command::type _from_type)
{
  math::vec2f normals[E];
  math::vec2f coordinates[E];

  const rx_size offset{m_elements.size()};
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
  RX_ASSERT(m_vertices.size() + E * 4 + (E -  2) * 3 <= 0xffffffff, "too many elements");

  for (rx_size i{0}, j{E-1}; i < E; j = i++) {
    const auto element{static_cast<rx_u32>(m_vertices.size())};

    m_elements.push_back(element + 0);
    m_elements.push_back(element + 1);
    m_elements.push_back(element + 2);
    m_elements.push_back(element + 2);
    m_elements.push_back(element + 3);
    m_elements.push_back(element + 0);

    m_vertices.push_back({_coordinates[i], {}, _color});
    m_vertices.push_back({_coordinates[j], {}, _color});
    m_vertices.push_back({coordinates[j], {}, {_color.r, _color.g, _color.b, 0.0f}});
    m_vertices.push_back({coordinates[i], {}, {_color.r, _color.g, _color.b, 0.0f}});
  }

  for (rx_size i{2}; i < E; i++) {
    const auto element{static_cast<rx_u32>(m_vertices.size())};

    m_elements.push_back(element + 0);
    m_elements.push_back(element + 1);
    m_elements.push_back(element + 2);

    m_vertices.push_back({_coordinates[0], {}, _color});
    m_vertices.push_back({_coordinates[(i - 1)], {}, _color});
    m_vertices.push_back({_coordinates[i], {}, _color});
  }

  add_batch(offset, _from_type);
}

void immediate::generate_line(const math::vec2f& _point_a,
  const math::vec2f& _point_b, rx_f32 _thickness, rx_f32 _roundness,
  const math::vec4f& _color)
{
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

  generate_polygon(vertices, _thickness, _color, queue::command::type::k_line);
}

void immediate::generate_rectangle(const math::vec2f& _position, const math::vec2f& _size,
  rx_f32 _roundness, const math::vec4f& _color)
{
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

    generate_polygon(vertices, 1.0f, _color, queue::command::type::k_rectangle);
  } else {
    const math::vec2f vertices[]{
      {_position.x,           _position.y},
      {_position.x + _size.w, _position.y},
      {_position.x + _size.w, _position.y + _size.h},
      {_position.x,           _position.y + _size.h}
    };
    
    generate_polygon(vertices, 1.0f, _color, queue::command::type::k_rectangle);
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

static rx_f32 calculate_text_length(immediate::font* _font, rx_f32 _scale,
  const string& _contents)
{
  // _scale /= static_cast<rx_f32>(_font->size());

  const rx_size length{_contents.size()};

  rx_f32 position{0.0f};
  rx_f32 span{0.0f};

  for (rx_size i{0}; i < length; i++) {
    const int ch{_contents[i]};
    if (ch == '^') {
      const char* next{_contents.data() + i + 1};
      if (*next != '^') {
        math::vec4f ignore;
        i += calculate_text_color(next, ignore);
        continue;
      }
    }

    const auto glyph{_font->glyph_for_code(ch - 32)};
    const auto round{static_cast<rx_s32>(math::floor(position + glyph.offset.x) + .5f)};

    span = static_cast<rx_f32>(round)
      + static_cast<rx_f32>(glyph.position[1].x) * _scale
      - static_cast<rx_f32>(glyph.position[0].x) * _scale;
    
    position += glyph.x_advance * _scale;
  }

  return span;
}

void immediate::generate_text(rx_s32 _size, const string& _font,
  const string& _contents, rx_f32 _scale, const math::vec2f& _position,
  text_align _align, const math::vec4f& _color)
{
  const font::key key{_size, _font};
  const auto find{m_fonts.find(key)};
  font* font_map{nullptr};
  if (find) {
    font_map = *find;
  } else {
    font_map = utility::allocate_and_construct<font>(m_frontend->allocator(),
      key, m_frontend);
    m_fonts.insert(key, font_map);
  }

  math::vec2f position{_position};
  math::vec4f color{_color};

  switch (_align) {
  case text_align::k_center:
    position.x -= calculate_text_length(font_map, _scale, _contents) * .5f;
    break;
  case text_align::k_right:
    position.x -= calculate_text_length(font_map, _scale, _contents);
    break;
  case text_align::k_left:
    break;
  }

  // scale relative to font size
  // _scale /= static_cast<rx_f32>(font_map->size());

  const rx_size offset{m_elements.size()};
  const rx_size length{_contents.size()};
  for (rx_size i{0}; i < length; i++) {
    const int ch{_contents[i]};
    if (ch == '^') {
      const char* next{_contents.data() + i + 1};
      if (*next != '^') {
        i += calculate_text_color(next, color);
        continue;
      }
    }

    const font::quad quad{font_map->quad_for_glyph(ch - 32, _scale, position)};
    const rx_u32 element{static_cast<rx_u32>(m_vertices.size())};

    m_elements.push_back(element + 0);
    m_elements.push_back(element + 1);
    m_elements.push_back(element + 2);
    m_elements.push_back(element + 0);
    m_elements.push_back(element + 3);
    m_elements.push_back(element + 1);

    m_vertices.push_back({quad.position[0], quad.coordinate[0], color});
    m_vertices.push_back({quad.position[1], quad.coordinate[1], color});
    m_vertices.push_back({{quad.position[1].x, quad.position[0].y}, {quad.coordinate[1].s, quad.coordinate[0].t}, color});
    m_vertices.push_back({{quad.position[0].x, quad.position[1].y}, {quad.coordinate[0].s, quad.coordinate[1].t}, color});
  }

  add_batch(offset, queue::command::type::k_text, font_map->texture());
}

void immediate::add_batch(rx_size _offset, queue::command::type _type,
  texture2D* _texture)
{
  const rx_size count{m_elements.size() - _offset};

  state render_state;
  render_state.blend.record_enable(true);
  render_state.blend.record_blend_factors(blend_state::factor_type::k_src_alpha,
    blend_state::factor_type::k_one_minus_src_alpha);
  
  render_state.depth.record_test(false);
  render_state.depth.record_write(false);

  render_state.cull.record_enable(false);

  render_state.scissor.record_enable(m_scissor_size.w > 0);
  render_state.scissor.record_offset(m_scissor_position);
  render_state.scissor.record_size(m_scissor_size);

  render_state.flush(); 

  if (!m_batches.is_empty()) {
    auto& batch{m_batches.last()};
    if (batch.kind == _type && batch.render_state == render_state && batch.texture == _texture) {
      batch.count += count;
      return;
    }
  }

  m_batches.push_back({_offset, count, _type, render_state, _texture});
}

} // namespace rx::render