#include <rx/render/immediate.h>
#include <rx/render/frontend.h>
#include <rx/render/buffer.h>
#include <rx/render/technique.h>

#include <rx/math/constants.h>
#include <rx/math/trig.h>

#include <stdlib.h> // abort

#include <rx/core/filesystem/file.h>
#include <rx/core/json.h>

namespace rx::render {

bool immediate::queue::command::operator!=(const command& _command) const {
  if (_command.hash != hash) {
    return true;
  }

  if (_command.type != type) {
    return true;
  }

  if (_command.flags != flags) {
    return true;
  }

  if (_command.color != color) {
    return true;
  }

  switch (type) {
  case command::category::k_line:
    return _command.as_line != as_line;
  case command::category::k_rectangle:
    return _command.as_rectangle != as_rectangle;
  case command::category::k_scissor:
    return _command.as_scissor != as_scissor;
  case command::category::k_triangle:
    return _command.as_triangle != as_triangle;
  }

  return false;
}

void immediate::queue::record_scissor(const math::vec2i& _position,
  const math::vec2i& _size)
{
  command next_command;
  next_command.type = command::category::k_scissor;
  next_command.flags = _position.x < 0 ? 0 : 1;
  next_command.color = {};
  next_command.as_scissor.position = _position;
  next_command.as_scissor.size = _size;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.type));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2i>{}(next_command.as_scissor.position));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2i>{}(next_command.as_scissor.size));

  m_commands.push_back(utility::move(next_command));
}

void immediate::queue::record_rectangle(const math::vec2i& _position,
  const math::vec2i& _size, rx_s32 _roundness, const math::vec4f& _color)
{
  command next_command;
  next_command.type = command::category::k_rectangle;
  next_command.flags = 0;
  next_command.color = _color;
  next_command.as_rectangle.position = _position;
  next_command.as_rectangle.size = _size;
  next_command.as_rectangle.roundness = _roundness;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.type));
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
  next_command.type = command::category::k_line;
  next_command.flags = 0;
  next_command.color = _color;
  next_command.as_line.points[0] = _point_a;
  next_command.as_line.points[1] = _point_b;
  next_command.as_line.roundness = _roundness;
  next_command.as_line.thickness = _thickness;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.type));

  m_commands.push_back(utility::move(next_command));
}

void immediate::queue::record_triangle(const math::vec2i& _position,
  const math::vec2i& _size, rx_u32 _flags, const math::vec4f& _color)
{
  command next_command;
  next_command.type = command::category::k_triangle;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_triangle.position = _position;
  next_command.as_triangle.size = _size;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.type));
  next_command.hash = hash_combine(next_command.hash, hash<rx_u32>{}(next_command.flags));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash<math::vec2i>{}(next_command.as_triangle.position);
  next_command.hash = hash<math::vec2i>{}(next_command.as_triangle.size);

  m_commands.push_back(utility::move(next_command));
}

bool immediate::queue::operator!=(const queue& _queue) const {
  if (_queue.m_commands.size() != m_commands.size()) {
    return true;
  }

  for (rx_size i{0}; i < m_commands.size(); i++) {
    if (_queue.m_commands[i] != m_commands[i]) {
      return true;
    }
  }

  return false;
}

void immediate::queue::clear() {
  m_commands.clear();
}

immediate::immediate(frontend* _frontend, technique* _technique)
  : m_frontend{_frontend}
  , m_technique{_technique}
  , m_rd_index{1}
  , m_wr_index{0}
{
  // generate circle geometry
  for (rx_size i{0}; i < k_circle_vertices; i++) {
    const rx_f32 phi{rx_f32(i) / rx_f32(k_circle_vertices) * math::k_pi<rx_f32> * 2.0f};
    m_circle_vertices[i] = {math::cos(phi), math::sin(phi)};
  }

  for (rx_size i{0}; i < k_buffers; i++) {
    m_buffers[i] = m_frontend->create_buffer(RX_RENDER_TAG("immediate"));
    m_buffers[i]->record_stride(sizeof(vertex));
    m_buffers[i]->record_element_type(buffer::element_category::k_u32);
    m_buffers[i]->record_attribute(buffer::attribute::category::k_f32, 2, offsetof(vertex, position));
    m_buffers[i]->record_attribute(buffer::attribute::category::k_f32, 2, offsetof(vertex, coordinate));
    m_buffers[i]->record_attribute(buffer::attribute::category::k_f32, 4, offsetof(vertex, color));
    m_frontend->initialize_buffer(RX_RENDER_TAG("immediate"), m_buffers[i]);
  }
}

immediate::~immediate() {
  for (rx_size i{0}; i < k_buffers; i++) {
    m_frontend->destroy_buffer(RX_RENDER_TAG("immediate"), m_buffers[i]);
  }
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
      switch (_command.type) {
      case queue::command::category::k_rectangle:
        generate_rectangle(
          _command.as_rectangle.position.cast<rx_f32>(),
          _command.as_rectangle.size.cast<rx_f32>(),
          static_cast<rx_f32>(_command.as_rectangle.roundness),
          _command.color);
        break;
      case queue::command::category::k_line:
        generate_line(
          _command.as_line.points[0].cast<rx_f32>(),
          _command.as_line.points[1].cast<rx_f32>(),
          static_cast<rx_f32>(_command.as_line.thickness),
          static_cast<rx_f32>(_command.as_line.roundness),
          _command.color);
        break;
      case queue::command::category::k_triangle:
        break;
      case queue::command::category::k_scissor:
        m_scissor_position = _command.as_scissor.position;
        m_scissor_size = _command.as_scissor.size;
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
    m_render_queue[m_wr_index] = m_queue;

    m_wr_index = (m_wr_index + 1) % k_buffers;
  }

  // if the last queue has any draw commands, render them now
  if (!last_empty) {
    m_render_batches[m_rd_index].each_fwd([&](const batch& _batch) {
      switch (_batch.category) {
      case queue::command::category::k_rectangle:
        [[fallthrough]];
      case queue::command::category::k_line:
        [[fallthrough]];
      case queue::command::category::k_triangle:
        m_frontend->draw_elements(
          RX_RENDER_TAG("immediate"),
          _batch.render_state,
          _target,
          m_buffers[m_rd_index],
          *m_technique,
          _batch.count,
          _batch.offset,
          primitive_type::k_triangles,
          "");
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
  const rx_f32 _thickness, const math::vec4f& _color, queue::command::category _from_type)
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

  generate_polygon(vertices, _thickness, _color, queue::command::category::k_line);
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

    generate_polygon(vertices, 1.0f, _color, queue::command::category::k_rectangle);
  } else {
    const math::vec2f vertices[]{
      {_position.x,           _position.y},
      {_position.x + _size.w, _position.y},
      {_position.x + _size.w, _position.y + _size.h},
      {_position.x,           _position.y + _size.h}
    };
    
    generate_polygon(vertices, 1.0f, _color, queue::command::category::k_rectangle);
  }
}

void immediate::add_batch(rx_size _offset, queue::command::category _category) {
  const rx_size count{m_elements.size() - _offset};

  state render_state;
  render_state.blend.record_enable(true);
  render_state.blend.record_blend_factors(blend_state::factor_type::k_src_alpha,
    blend_state::factor_type::k_one_minus_src_alpha);
  
  render_state.depth.record_test(false);
  render_state.depth.record_write(false);

  render_state.scissor.record_enable(m_scissor_size.w > 0);
  render_state.scissor.record_offset(m_scissor_position);
  render_state.scissor.record_size(m_scissor_size);

  render_state.flush(); 

  if (!m_batches.is_empty()) {
    auto& batch{m_batches.last()};
    if (batch.category == _category && batch.render_state == render_state) {
      batch.count += count;
      return;
    }
  }

  m_batches.push_back({_offset, count, _category, render_state});
}

} // namespace rx::render