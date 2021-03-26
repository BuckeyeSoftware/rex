#include <string.h> // strchr
#include <stdio.h> // sscanf
#include <inttypes.h> // PRIx32

#include "rx/render/immediate2D.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/buffer.h"
#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/target.h"

#include "rx/core/math/constants.h"
#include "rx/core/math/sin.h"
#include "rx/core/math/cos.h"

#include "rx/core/filesystem/file.h"
#include "rx/core/profiler.h"
#include "rx/core/json.h"

#include "rx/texture/chain.h"

#include "lib/stb_truetype.h"

namespace Rx::Render {

// [Immediate2D::Queue]
Immediate2D::Queue::Queue(Memory::Allocator& _allocator)
  : m_commands{_allocator}
  , m_string_table{_allocator}
{
}

Immediate2D::Queue::Queue(Queue&& queue_)
  : m_commands{Utility::move(queue_.m_commands)}
  , m_string_table{Utility::move(queue_.m_string_table)}
{
}

Immediate2D::Queue& Immediate2D::Queue::operator=(Queue&& queue_) {
  m_commands = Utility::move(queue_.m_commands);
  m_string_table = Utility::move(queue_.m_string_table);
  return *this;
}

bool Immediate2D::Queue::record_scissor(const Math::Vec2f& _position,
  const Math::Vec2f& _size)
{
  RX_PROFILE_CPU("immediate2D::queue::record_scissor");

  if (_position.x < 0.0f) {
    m_scissor = nullopt;
  } else {
    m_scissor = Box{_position, _size};
  }

  Command next_command;
  next_command.type = Command::Type::SCISSOR;
  next_command.flags = _position.x < 0.0f ? 0 : 1;
  next_command.color = {};
  next_command.as_scissor.position = _position;
  next_command.as_scissor.size = _size;
  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate2D::Queue::record_rectangle(
  const Math::Vec2f& _position, const Math::Vec2f& _size, Float32 _roundness,
  const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::queue::record_rectangle");

  Command next_command;
  next_command.type = Command::Type::RECTANGLE;
  next_command.flags = 0;
  next_command.color = _color;
  next_command.as_rectangle.position = _position;
  next_command.as_rectangle.size = _size;
  next_command.as_rectangle.roundness = _roundness;

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate2D::Queue::record_line(
  const Math::Vec2f& _point_a, const Math::Vec2f& _point_b, Float32 _roundness,
  Float32 _thickness, const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::queue::record_line");

  Command next_command;
  next_command.type = Command::Type::LINE;
  next_command.flags = 0;
  next_command.color = _color;
  next_command.as_line.points[0] = _point_a;
  next_command.as_line.points[1] = _point_b;
  next_command.as_line.roundness = _roundness;
  next_command.as_line.thickness = _thickness;

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate2D::Queue::record_triangle(
  const Math::Vec2f& _position, const Math::Vec2f& _size, Uint32 _flags,
  const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::queue::record_triangle");

  Command next_command;
  next_command.type = Command::Type::TRIANGLE;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_triangle.position = _position;
  next_command.as_triangle.size = _size;

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate2D::Queue::record_text(
  const char* _font, Size _font_length, const Math::Vec2f& _position,
  Sint32 _size, Float32 _scale, TextAlign _align, const char* _text,
  Size _text_length, const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::queue::record_text");

  if (_text_length == 0) {
    return false;
  }

  // Quick and dirty rejection of text outside the scissor.
  if (m_scissor) {
    // The text is above the scissor rectangle.
    if (_position.y > m_scissor->position.y + m_scissor->size.h) {
      return false;
    }

    // The text is below the scissor rectangle.
    if (_position.y < m_scissor->position.y) {
      return false;
    }

    // Text is outside the right edge of the scissor while not right aligned.
    if (_align != TextAlign::RIGHT
      && _position.x > m_scissor->position.x + m_scissor->size.w) {
      return false;
    }
  }

  Command next_command;
  next_command.type = Command::Type::TEXT;
  next_command.flags = static_cast<Uint32>(_align);
  next_command.color = _color;
  next_command.as_text.position = _position;
  next_command.as_text.size = _size;
  next_command.as_text.scale = _scale;
  next_command.as_text.font_length = _font_length;
  next_command.as_text.text_length = _text_length;

  // insert strings into string table
  const auto font_index = m_string_table.add(_font, _font_length);
  const auto text_index = m_string_table.add(_text, _text_length);
  if (!font_index || !text_index) {
    return false;
  }

  next_command.as_text.font_index = *font_index;
  next_command.as_text.text_index = *text_index;

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate2D::Queue::record_text(
  const char* _font, const Math::Vec2f& _position, Sint32 _size, Float32 _scale,
  TextAlign _align, const char* _contents, const Math::Vec4f& _color)
{
  return record_text(_font, strlen(_font), _position, _size, _scale, _align,
    _contents, strlen(_contents), _color);
}

void Immediate2D::Queue::clear() {
  m_commands.clear();
  m_string_table.clear();
}

// [Immediate2D::Font]
Immediate2D::Font::Font(const Key& _key, Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_size{_key.size}
  , m_resolution{DEFAULT_RESOLUTION}
  , m_texture{nullptr}
  , m_glyphs{m_frontend->allocator()}
{
  auto name = String::format("base/fonts/%s.ttf", _key.name);
  auto data = Filesystem::read_binary_file(m_frontend->allocator(), name);

  if (data) {
    static constexpr const int GLYPHS = 96; // all of ascii

    // figure out the atlas size needed
    for (;;) {
      Vector<stbtt_bakedchar> baked_glyphs(m_frontend->allocator());
      RX_ASSERT(baked_glyphs.resize(GLYPHS), "out of memory");
      LinearBuffer baked_atlas{m_frontend->allocator()};
      RX_ASSERT(baked_atlas.resize(m_resolution * m_resolution), "out of memory");

      const int result{stbtt_BakeFontBitmap(data->data(), 0,
        static_cast<Float32>(m_size), baked_atlas.data(),
        static_cast<int>(m_resolution), static_cast<int>(m_resolution), 32,
        GLYPHS, baked_glyphs.data())};

      if (result == -GLYPHS || result > 0) {
        // create a texture chain from this baked font bitmap
        Rx::Texture::Chain chain{m_frontend->allocator()};
        chain.generate(
                baked_atlas.data(),
                Rx::Texture::PixelFormat::R_U8,
                Rx::Texture::PixelFormat::R_U8,
                {m_resolution, m_resolution},
                false, true);

        // create and upload baked atlas
        m_texture = m_frontend->create_texture2D(RX_RENDER_TAG("font"));
        m_texture->record_format(Frontend::Texture::DataFormat::R_U8);
        m_texture->record_type(Frontend::Texture::Type::STATIC);
        m_texture->record_levels(chain.levels().size());
        m_texture->record_dimensions({m_resolution, m_resolution});
        m_texture->record_filter({true, false, true});
        m_texture->record_wrap({Frontend::Texture::WrapType::CLAMP_TO_EDGE,
                                Frontend::Texture::WrapType::CLAMP_TO_EDGE});

        const auto& levels{chain.levels()};
        for (Size i{0}; i < levels.size(); i++) {
          const auto& level{levels[i]};
          m_texture->write(chain.data().data() + level.offset, i);
        }

        m_frontend->initialize_texture(RX_RENDER_TAG("font"), m_texture);

        // copy glyph information
        RX_ASSERT(m_glyphs.resize(GLYPHS), "out of memory");

        for (int i{0}; i < GLYPHS; i++) {
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

Immediate2D::Font::Font(Font&& font_)
  : m_frontend{font_.m_frontend}
  , m_size{Utility::exchange(font_.m_size, 0)}
  , m_resolution{Utility::exchange(font_.m_resolution, DEFAULT_RESOLUTION)}
  , m_texture{Utility::exchange(font_.m_texture, nullptr)}
  , m_glyphs{Utility::move(font_.m_glyphs)}
{
}

Immediate2D::Font::~Font() {
  m_frontend->destroy_texture(RX_RENDER_TAG("font"), m_texture);
}

Immediate2D::Font::Quad Immediate2D::Font::quad_for_glyph(Size _glyph,
  Float32 _scale, Math::Vec2f& position_) const
{
  const auto& glyph{m_glyphs[_glyph]};

  const Math::Vec2f scaled_offset{glyph.offset * _scale};
  const Math::Vec2f scaled_position[2]{
    glyph.position[0].cast<Float32>() * _scale,
    glyph.position[1].cast<Float32>() * _scale
  };

  const Math::Vec2f round{
    position_.x + scaled_offset.x,
    position_.y - scaled_offset.y
  };

  Quad result;

  result.position[0] = round;
  result.position[1] = {
    round.x + scaled_position[1].x - scaled_position[0].x,
    round.y - scaled_position[1].y + scaled_position[0].y
  };

  result.coordinate[0] = glyph.position[0].cast<Float32>() / static_cast<Float32>(m_resolution);
  result.coordinate[1] = glyph.position[1].cast<Float32>() / static_cast<Float32>(m_resolution);

  position_.x += glyph.x_advance * _scale;

  return result;
}

// [Immediate2D]
Optional<Immediate2D> Immediate2D::create(Frontend::Context* _frontend) {
  auto technique = _frontend->find_technique_by_name("immediate2D");
  if (!technique) {
    return nullopt;
  }

  Frontend::Buffer::Format format;
  format.record_type(Frontend::Buffer::Type::DYNAMIC);
  format.record_element_type(Frontend::Buffer::ElementType::U32);
  format.record_vertex_stride(sizeof(Vertex));
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x2, offsetof(Vertex, position)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x2, offsetof(Vertex, coordinate)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x4, offsetof(Vertex, color)});
  format.finalize();

  Array<Frontend::Buffer*[BUFFERS]> buffers;
  for (Size i = 0; i < BUFFERS; i++) {
    auto buffer = _frontend->create_buffer(RX_RENDER_TAG("immediate2D"));
    if (buffer) {
      buffer->record_format(format);
      _frontend->initialize_buffer(RX_RENDER_TAG("immediate2D"), buffer);
      buffers[i] = buffer;
    } else {
      for (Size j = 0; j < i; j++) {
        _frontend->destroy_buffer(RX_RENDER_TAG("immediate2D"), buffer);
      }
      return nullopt;
    }
  }

  return Immediate2D {
    _frontend,
    technique,
    FontMap {
      _frontend->allocator()
    },
    Queue {
      _frontend->allocator()
    },
    Vector<Batch> {
      _frontend->allocator()
    },
    RenderBatches {
      _frontend->allocator(),
      _frontend->allocator()
    },
    RenderQueues {
      _frontend->allocator(),
      _frontend->allocator()
    },
    Utility::move(buffers)
  };
}

Immediate2D::Immediate2D(Frontend::Context* _frontend,
  Frontend::Technique* _technique, FontMap&& font_map_, Queue&& queue_,
  Vector<Batch>&& batches_, RenderBatches&& render_batches_,
  RenderQueues&& render_queues_, Buffers&& buffers_)
    : m_frontend{_frontend}
    , m_technique{_technique}
    , m_fonts{Utility::move(font_map_)}
    , m_queue{Utility::move(queue_)}
    , m_vertices{nullptr}
    , m_elements{nullptr}
    , m_batches{Utility::move(batches_)}
    , m_vertex_index{0}
    , m_element_index{0}
    , m_rd_index{1}
    , m_wr_index{0}
    , m_render_batches{Utility::move(render_batches_)}
    , m_render_queues{Utility::move(render_queues_)}
    , m_buffers{Utility::move(buffers_)}
{
  // Generate circle geometry.
  for (Size i = 0; i < CIRCLE_VERTICES; i++) {
    const auto phi = Float32(i) / Float32(CIRCLE_VERTICES) * Math::PI<Float32> * 2.0f;
    m_circle_vertices[i] = {Math::cos(phi), Math::sin(phi)};
  }
}

void Immediate2D::release() {
  if (m_frontend) {
    for (Size i = 0; i < BUFFERS; i++) {
      m_frontend->destroy_buffer(RX_RENDER_TAG("immediate2D"), m_buffers[i]);
    }
  }
}

void Immediate2D::Immediate2D::render(Frontend::Target* _target) {
  RX_PROFILE_CPU("immediate2D::render");

  // avoid rendering if the last update did not produce any draw commands and
  // this iteration has no updates either
  const bool last_empty{m_render_queues[m_rd_index].is_empty()};
  if (last_empty && m_queue.is_empty()) {
    return;
  }

  // calculate storage needed
  Size n_vertices{0};
  Size n_elements{0};
  m_queue.m_commands.each_fwd([&](const Queue::Command& _command) {
    switch (_command.type) {
    case Queue::Command::Type::RECTANGLE:
      size_rectangle(
        _command.as_rectangle.roundness,
        n_vertices,
        n_elements);
        break;
    case Queue::Command::Type::LINE:
      size_line(_command.as_line.roundness, n_vertices, n_elements);
      break;
    case Queue::Command::Type::TRIANGLE:
      size_triangle(n_vertices, n_elements);
      break;
    case Queue::Command::Type::TEXT:
      size_text(
        m_queue.m_string_table[_command.as_text.text_index],
        _command.as_text.text_length,
        n_vertices,
        n_elements);
      break;
    default:
      break;
    }
  });

  // The commands generated did not produce any primitives.
  if (n_elements == 0) {
    return;
  }

  // allocate storage
  m_vertices = (Vertex*)m_buffers[m_wr_index]->map_vertices(n_vertices * sizeof(Vertex));
  m_elements = (Uint32*)m_buffers[m_wr_index]->map_elements(n_elements * sizeof(Uint32));

  // generate geometry for a future frame
  m_queue.m_commands.each_fwd([this](const Queue::Command& _command) {
    switch (_command.type) {
    case Queue::Command::Type::RECTANGLE:
      generate_rectangle(
        _command.as_rectangle.position.cast<Float32>(),
        _command.as_rectangle.size.cast<Float32>(),
        static_cast<Float32>(_command.as_rectangle.roundness),
        _command.color);
      break;
    case Queue::Command::Type::LINE:
      generate_line(
        _command.as_line.points[0].cast<Float32>(),
        _command.as_line.points[1].cast<Float32>(),
        static_cast<Float32>(_command.as_line.thickness),
        static_cast<Float32>(_command.as_line.roundness),
        _command.color);
      break;
    case Queue::Command::Type::TRIANGLE:
      generate_triangle(
        _command.as_triangle.position.cast<Float32>(),
        _command.as_triangle.size.cast<Float32>(),
        _command.color);
      break;
    case Queue::Command::Type::TEXT:
      generate_text(
        _command.as_text.size,
        m_queue.m_string_table[_command.as_text.font_index],
        _command.as_text.font_length,
        m_queue.m_string_table[_command.as_text.text_index],
        _command.as_text.text_length,
        _command.as_text.scale,
        _command.as_text.position.cast<Float32>(),
        static_cast<TextAlign>(_command.flags),
        _command.color);
      break;
    case Queue::Command::Type::SCISSOR:
      m_scissor_position = _command.as_scissor.position.cast<Sint32>();
      m_scissor_size = _command.as_scissor.size.cast<Sint32>();
      break;
    default:
      break;
    }
  });

  // record the edit
  m_buffers[m_wr_index]->record_vertices_edit(0, n_vertices * sizeof(Vertex));
  m_buffers[m_wr_index]->record_elements_edit(0, n_elements * sizeof(Uint32));
  m_frontend->update_buffer(RX_RENDER_TAG("immediate2D"), m_buffers[m_wr_index]);

  // clear staging buffers
  m_vertices = nullptr;
  m_elements = nullptr;

  m_vertex_index = 0;
  m_element_index = 0;

  // write buffer will be processed some time in the future
  m_render_batches[m_wr_index] = Utility::move(m_batches);
  m_render_queues[m_wr_index] = Utility::move(m_queue);

  m_wr_index = (m_wr_index + 1) % BUFFERS;

  // if the last queue has any draw commands, render them now
  const auto& dimensions{_target->dimensions().cast<Sint32>()};
  m_technique->configuration(0).variant(0)->uniforms()[0].record_vec2i(dimensions);
  m_technique->configuration(0).variant(1)->uniforms()[0].record_vec2i(dimensions);

  if (!last_empty) {
    m_render_batches[m_rd_index].each_fwd([&](Batch& _batch) {
      _batch.render_state.viewport.record_dimensions(_target->dimensions());

      Frontend::Buffers draw_buffers;
      draw_buffers.add(0);

      Frontend::Textures draw_textures;

      switch (_batch.type) {
      case Batch::Type::TRIANGLES:
        m_frontend->draw(
          RX_RENDER_TAG("immediate2D triangles"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->configuration(0).variant(0),
          _batch.count,
          _batch.offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::TRIANGLES,
          {});
          break;
      case Batch::Type::LINES:
        m_frontend->draw(
          RX_RENDER_TAG("immediate2D lines"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->configuration(0).variant(0),
          _batch.count,
          _batch.offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::LINES,
          {});
          break;
      case Batch::Type::TEXT:
        draw_textures.clear();
        draw_textures.add(_batch.texture);

        m_frontend->draw(
          RX_RENDER_TAG("immediate2D text"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->configuration(0).variant(1),
          _batch.count,
          _batch.offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::TRIANGLES,
          draw_textures);
        break;
      default:
        break;
      }
    });

    m_rd_index = (m_rd_index + 1) % BUFFERS;
  }

  m_queue.clear();
}

template<Size E>
void Immediate2D::generate_polygon(const Math::Vec2f (&_coordinates)[E],
                                   const Float32 _thickness, const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::generate_polygon");

  Math::Vec2f normals[E];
  Math::Vec2f coordinates[E];

  const Size offset{m_element_index};

  for (Size i{0}, j{E - 1}; i < E; j = i++) {
    const Math::Vec2f& f0{coordinates[j]};
    const Math::Vec2f& f1{coordinates[i]};
    const Math::Vec2f delta{Math::normalize(f1 - f0)};
    normals[i] = {delta.y, -delta.x};
  }

  for (Size i{0}, j{E - 1}; i < E; j = i++) {
    const Math::Vec2f& f0{normals[j]};
    const Math::Vec2f& f1{normals[i]};
    const Math::Vec2f normal{Math::normalize((f0 + f1) * 0.5f)};
    coordinates[i] = _coordinates[i] + normal * _thickness;
  }

  // sanity check that we don't exceed the element format
  RX_ASSERT(m_vertex_index + E * 4 + (E -  2) * 3 <= 0xffffffff, "too many elements");

  for (Size i{0}, j{E - 1}; i < E; j = i++) {
    const auto element{static_cast<Uint32>(m_vertex_index)};

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

  for (Size i{2}; i < E; i++) {
    const auto element{static_cast<Uint32>(m_vertex_index)};

    add_element(element + 0);
    add_element(element + 1);
    add_element(element + 2);

    add_vertex({_coordinates[0], {}, _color});
    add_vertex({_coordinates[(i - 1)], {}, _color});
    add_vertex({_coordinates[i], {}, _color});
  }

  add_batch(offset, Batch::Type::TRIANGLES, _color.a < 1.0f);
}

void Immediate2D::generate_rectangle(const Math::Vec2f& _position, const Math::Vec2f& _size,
                                     Float32 _roundness, const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::generate_rectangle");

  if (_roundness > 0.0f) {
    static constexpr const auto ROUND = CIRCLE_VERTICES / 4;
    Math::Vec2f vertices[(ROUND + 1) * 4];

    Size j{0};
    for (Size i{0}; i <= ROUND; i++) {
      vertices[j++] = _position + _size - _roundness + m_circle_vertices[i] * _roundness;
    }

    for (Size i = ROUND; i <= ROUND * 2; i++) {
      vertices[j++] = _position + Math::Vec2f{_roundness, _size.h - _roundness} + m_circle_vertices[i] * _roundness;
    }

    for (Size i = ROUND * 2; i <= ROUND * 3; i++) {
      vertices[j++] = _position + _roundness + m_circle_vertices[i] * _roundness;
    }

    for (Size i = ROUND * 3; i < ROUND * 4; i++) {
      vertices[j++] = _position + Math::Vec2f{_size.w - _roundness, _roundness} + m_circle_vertices[i] * _roundness;
    }

    vertices[j++] = _position + Math::Vec2f{_size.w - _roundness, _roundness} + m_circle_vertices[0] * _roundness;

    generate_polygon(vertices, 1.0f, _color);
  } else {
    const Math::Vec2f vertices[]{
      {_position.x,           _position.y},
      {_position.x + _size.w, _position.y},
      {_position.x + _size.w, _position.y + _size.h},
      {_position.x,           _position.y + _size.h}
    };

    generate_polygon(vertices, 1.0f, _color);
  }
}

void Immediate2D::generate_line(const Math::Vec2f& _point_a,
                                const Math::Vec2f& _point_b, Float32 _thickness, Float32 _roundness,
                                const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::generate_line");

  if (_roundness > 0.0f) {
    Math::Vec2f delta{Math::normalize(_point_b - _point_a)};
    Math::Vec2f normal{delta.y, -delta.x};

    _roundness -= _thickness;
    _roundness *= 0.5f;

    delta = delta * Math::Vec2f{_roundness, _roundness};
    normal = normal * Math::Vec2f{_roundness, _roundness};

    const Math::Vec2f vertices[]{
      _point_a - delta - normal,
      _point_a - delta + normal,
      _point_b + delta + normal,
      _point_b + delta - normal
    };

    generate_polygon(vertices, _thickness, _color);
  } else {
    const Size offset{m_element_index};
    const auto element{static_cast<Uint32>(m_vertex_index)};

    add_element(element + 0);
    add_element(element + 1);

    add_vertex({_point_a, {}, _color});
    add_vertex({_point_b, {}, _color});

    add_batch(offset, Batch::Type::LINES, _color.a < 1.0f);
  }
}

static Size calculate_text_color(const char* _contents, Math::Vec4f& color_) {
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
    Uint32 color_hex{0};
    sscanf(_contents, "[%" PRIx32 "]", &color_hex);

    const auto a{static_cast<Float32>(color_hex & 0xff) / 255.0f};
    const auto b{static_cast<Float32>((color_hex >> 8) & 0xff) / 255.0f};
    const auto g{static_cast<Float32>((color_hex >> 16) & 0xff) / 255.0f};
    const auto r{static_cast<Float32>((color_hex >> 24) & 0xff) / 255.0f};

    color_ = { r, g, b, a };
    return terminate - _contents + 1;
  }

  return 0;
}

static Float32 calculate_text_length(Ptr<Immediate2D::Font>& _font, Float32 _scale,
                                    const char* _text, Size _text_length)
{
  Float32 position{0.0f};

  for (Size i{0}; i < _text_length; i++) {
    const int ch{_text[i]};
    if (ch == '^') {
      const char* next{_text + i + 1};
      if (*next != '^') {
        Math::Vec4f ignore;
        i += calculate_text_color(next, ignore);
        continue;
      }
    }

    const auto glyph{_font->glyph_for_code(ch - 32)};
    position += glyph.x_advance * _scale;
  }

  return position;
}

Float32 Immediate2D::measure_text_length(const char* _font,
                                        const char* _text, Size _text_length, Sint32 _size, Float32 _scale)
{
  RX_PROFILE_CPU("immediate2D::measure_text_length");

  auto& font_map = access_font({_size, _font});
  return calculate_text_length(font_map, _scale, _text, _text_length);
}

void Immediate2D::generate_text(Sint32 _size, const char* _font,
                                Size _font_length, const char* _contents, Size _contents_length,
                                Float32 _scale, const Math::Vec2f& _position, TextAlign _align,
                                const Math::Vec4f& _color)
{
  RX_PROFILE_CPU("immediate2D::generate_text");

  (void)_font_length;

  auto& font_map = access_font({_size, _font});

  Math::Vec2f position{_position};
  Math::Vec4f color{_color};

  switch (_align) {
  case TextAlign::CENTER:
    position.x -= calculate_text_length(font_map, _scale, _contents, _contents_length) * .5f;
    break;
  case TextAlign::RIGHT:
    position.x -= calculate_text_length(font_map, _scale, _contents, _contents_length);
    break;
  case TextAlign::LEFT:
    break;
  }

  // scale relative to font size
  // _scale /= static_cast<Float32>(font_map->size());

  const Size offset{m_element_index};
  for (Size i{0}; i < _contents_length; i++) {
    const int ch{_contents[i]};
    if (ch == '^') {
      const char* next{_contents + i + 1};
      if (*next != '^') {
        i += calculate_text_color(next, color);
        continue;
      }
    }

    const Font::Quad quad =
      font_map->quad_for_glyph(ch - 32, _scale, position);

    const auto element =
      static_cast<Uint32>(m_vertex_index);

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

  add_batch(offset, Batch::Type::TEXT, true, font_map->texture());
}

void Immediate2D::generate_triangle(const Math::Vec2f& _position,
                                    const Math::Vec2f& _size, const Math::Vec4f& _color)
{
  const Math::Vec2f coordinates[]{
    _position,
    {_position.x + _size.w, _position.y + _size.h / 2.0f},
    {_position.x, _position.y + _size.h}
  };
  generate_polygon(coordinates, 1.0f, _color);
}

template<Size E>
void Immediate2D::size_polygon(Size& n_vertices_, Size& n_elements_) {
  n_vertices_ += 4 * E + 3 * (E - 2);
  n_elements_ += 6 * E + 3 * (E - 2);
}

void Immediate2D::size_rectangle(Float32 _roundness, Size& n_vertices_, Size& n_elements_) {
  if (_roundness > 0.0f) {
    static constexpr const auto ROUND = CIRCLE_VERTICES / 4;
    size_polygon<(ROUND + 1) * 4>(n_vertices_, n_elements_);
  } else {
    size_polygon<4>(n_vertices_, n_elements_);
  }
}

void Immediate2D::size_line(Float32 _roundness, Size& n_vertices_, Size& n_elements_) {
  if (_roundness > 0.0f) {
    size_polygon<4>(n_vertices_, n_elements_);
  } else {
    n_vertices_ += 2;
    n_elements_ += 2;
  }
}

void Immediate2D::size_text(const char* _contents, Size _contents_length,
                            Size& n_vertices_, Size& n_elements_)
{
  for (Size i{0}; i < _contents_length; i++) {
    const int ch{_contents[i]};
    if (ch == '^') {
      const char* next{_contents + i + 1};
      if (*next != '^') {
        Math::Vec4f sink;
        i += calculate_text_color(next, sink);
        continue;
      }
    }

    n_vertices_ += 4;
    n_elements_ += 6;
  }
}

void Immediate2D::size_triangle(Size& n_vertices_, Size& n_elements_) {
  size_polygon<3>(n_vertices_, n_elements_);
}

bool Immediate2D::add_batch(Size _offset, Batch::Type _type, bool _blend,
                            Frontend::Texture2D* _texture)
{
  RX_PROFILE_CPU("immediate2D::add_batch");

  const Size count{m_element_index - _offset};
  if (count == 0) {
    // Generated no geometry for this batch, discard it.
    return true;
  }

  Frontend::State render_state;

  if (_blend) {
    render_state.blend.record_enable(true);
    render_state.blend.record_blend_factors(
      Frontend::BlendState::FactorType::SRC_ALPHA,
      Frontend::BlendState::FactorType::ONE_MINUS_SRC_ALPHA);
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
    if (batch.render_state == render_state && batch.type == _type && batch.texture == _texture) {
      batch.count += count;
      return true;
    }
  }

  return m_batches.emplace_back(_offset, count, _type, render_state, _texture);
}

void Immediate2D::add_element(Uint32 _element) {
  m_elements[m_element_index++] = _element;
}

void Immediate2D::add_vertex(Vertex&& vertex_) {
  m_vertices[m_vertex_index++] = Utility::move(vertex_);
}

Ptr<Immediate2D::Font>& Immediate2D::access_font(const Font::Key& _key) {
  const auto find = m_fonts.find(_key);
  if (find) {
    return *find;
  }

  auto& allocator = m_frontend->allocator();
  auto new_font = make_ptr<Font>(allocator, _key, m_frontend);
  RX_ASSERT(new_font, "out of memory");

  return *m_fonts.insert(_key, Utility::move(new_font));
}

} // namespace Rx::Render::Frontend
