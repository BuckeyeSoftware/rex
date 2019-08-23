#include <stddef.h> 

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/buffer.h"

#include "rx/render/immediate3D.h"

namespace rx::render {

immediate3D::queue::queue(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_commands{m_allocator}
{
}

immediate3D::queue::queue(queue&& queue_)
  : m_allocator{queue_.m_allocator}
  , m_commands{utility::move(queue_.m_commands)}
{
  queue_.m_allocator = nullptr;
}

immediate3D::queue& immediate3D::queue::operator=(queue&& queue_) {
  RX_ASSERT(this != &queue_, "move from self");
  m_allocator = queue_.m_allocator;
  m_commands = utility::move(queue_.m_commands);
  queue_.m_allocator = nullptr;
  return *this;
}

bool immediate3D::queue::command::operator!=(const command& _command) const {
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
  case command::type::k_point:
    return _command.as_point != as_point;
  case command::type::k_line:
    return _command.as_line != as_line;
  case command::type::k_solid_sphere:
    return _command.as_solid_sphere != as_solid_sphere;
  }

  return false;
}

void immediate3D::queue::record_point(const math::vec3f& _point,
  const math::vec4f& _color, rx_f32 _size, rx_u8 _flags)
{
  command next_command;
  next_command.kind = command::type::k_point;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_point.position = _point;
  next_command.as_point.size = _size;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash<rx_u32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec3f>{}(next_command.as_point.position));
  next_command.hash = hash_combine(next_command.hash, hash<rx_f32>{}(next_command.as_point.size));

  m_commands.push_back(utility::move(next_command));
}

void immediate3D::queue::record_line(const math::vec3f& _point_a,
  const math::vec3f& _point_b, const math::vec4f& _color, rx_u8 _flags)
{
  command next_command;
  next_command.kind = command::type::k_line;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_line.point_a = _point_a;
  next_command.as_line.point_b = _point_b;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash<rx_u32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec3f>{}(next_command.as_line.point_a));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec3f>{}(next_command.as_line.point_b));

  m_commands.push_back(utility::move(next_command));
}

void immediate3D::queue::record_solid_sphere(const math::vec2f& _slices_and_stacks,
  const math::vec4f& _color, const math::mat4x4f& _transform, rx_u8 _flags)
{
  command next_command;
  next_command.kind = command::type::k_solid_sphere;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_solid_sphere.slices_and_stacks = _slices_and_stacks;
  next_command.as_solid_sphere.transform = _transform;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash<rx_u32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::vec2f>{}(next_command.as_solid_sphere.slices_and_stacks));
  next_command.hash = hash_combine(next_command.hash, hash<math::mat4x4f>{}(next_command.as_solid_sphere.transform));

  m_commands.push_back(utility::move(next_command));
}

bool immediate3D::queue::operator!=(const queue& _queue) const {
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

void immediate3D::queue::clear() {
  m_commands.clear();
}

immediate3D::immediate3D(frontend::interface* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("immediate3D")}
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

  for (rx_size i{0}; i < k_buffers; i++) {
    m_buffers[i] = m_frontend->create_buffer(RX_RENDER_TAG("immediate3D"));
    m_buffers[i]->record_stride(sizeof(vertex));
    m_buffers[i]->record_type(frontend::buffer::type::k_dynamic);
    m_buffers[i]->record_element_type(frontend::buffer::element_type::k_u32);
    m_buffers[i]->record_attribute(frontend::buffer::attribute::type::k_f32, 3, offsetof(vertex, position));
    m_buffers[i]->record_attribute(frontend::buffer::attribute::type::k_f32, 1, offsetof(vertex, size));
    m_buffers[i]->record_attribute(frontend::buffer::attribute::type::k_f32, 4, offsetof(vertex, color));
    m_frontend->initialize_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[i]);
  }
}

immediate3D::~immediate3D() {
  for(rx_size i{0}; i < k_buffers; i++) {
    m_frontend->destroy_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[i]);
  }
}

void immediate3D::render(frontend::target* _target, const math::mat4x4f& _view,
  const math::mat4x4f& _projection)
{
  // avoid renderinf if the last update did not produce any draw commands and
  // this iteration has no updates either
  const bool last_empty{m_render_queue[m_rd_index].is_empty()};
  if (last_empty && m_queue.is_empty()) {
    return;
  }

  // avoid generation geometry and uploading if the contents didn't change
  if (m_queue != m_render_queue[m_rd_index]) {
    m_queue.m_commands.each_fwd([this](const queue::command& _command) {
      switch (_command.kind) {
      case queue::command::type::k_point:
        generate_point(
          _command.as_point.position,
          _command.as_point.size,
          _command.color,
          _command.flags);
        break;
      case queue::command::type::k_line:
        generate_line(
          _command.as_line.point_a,
          _command.as_line.point_b,
          _command.color,
          _command.flags);
        break;
      case queue::command::type::k_solid_sphere:
        generate_solid_sphere(
          _command.as_solid_sphere.slices_and_stacks,
          _command.as_solid_sphere.transform,
          _command.color,
          _command.flags);
        break;
      default:
        break;
      }
    });

    // upload the geometry
    m_buffers[m_wr_index]->write_vertices(m_vertices.data(), m_vertices.size() * sizeof(vertex));
    m_buffers[m_wr_index]->write_elements(m_elements.data(), m_elements.size() * sizeof(rx_u32));
  
    m_frontend->update_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[m_wr_index]);

    // clear staging buffers
    m_vertices.clear();
    m_elements.clear();

    // write buffer will be processed some time in the future
    m_render_batches[m_wr_index] = utility::move(m_batches);
    m_render_queue[m_wr_index] = utility::move(m_queue);

    m_wr_index = (m_wr_index + 1) % k_buffers;
  }

  // if the last queue has any draw commands, render them now
  if (!last_empty) {
    m_technique->variant(0)->uniforms()[0].record_mat4x4f(_view);
    m_technique->variant(0)->uniforms()[1].record_mat4x4f(_projection);

    m_technique->variant(1)->uniforms()[0].record_mat4x4f(_view);
    m_technique->variant(1)->uniforms()[1].record_mat4x4f(_projection);

    // process the batches
    m_render_batches[m_rd_index].each_fwd([&](const batch& _batch) {
      switch (_batch.kind) {
      case queue::command::type::k_uninitialized:
        RX_HINT_UNREACHABLE();
        break;
      case queue::command::type::k_point:
        m_frontend->draw_elements(
          RX_RENDER_TAG("immediate3D points"),
          _batch.render_state,
          _target,
          m_buffers[m_rd_index],
          m_technique->variant(1),
          _batch.count,
          _batch.offset,
          frontend::primitive_type::k_points,
          "");
        break;
      case queue::command::type::k_line:
        m_frontend->draw_elements(
          RX_RENDER_TAG("immediate3D lines"),
          _batch.render_state,
          _target,
          m_buffers[m_rd_index],
          m_technique->variant(0),
          _batch.count,
          _batch.offset,
          frontend::primitive_type::k_lines,
          "");
        break;
      case queue::command::type::k_solid_sphere:
        m_frontend->draw_elements(
          RX_RENDER_TAG("immediate3D triangles"),
          _batch.render_state,
          _target,
          m_buffers[m_rd_index],
          m_technique->variant(0),
          _batch.count,
          _batch.offset,
          frontend::primitive_type::k_triangles,
          "");
        break;
      }
    });
    m_rd_index = (m_rd_index + 1) % k_buffers;
  }

  // clear the queue for the next frame
  m_queue.clear();
}

void immediate3D::generate_point(const math::vec3f& _position, rx_f32 _size,
  const math::vec4f& _color, rx_u32 _flags)
{
  const auto offset{m_elements.size()};
  const auto element{static_cast<rx_u32>(m_vertices.size())};

  m_elements.push_back(element + 0);

  m_vertices.push_back({_position, _size, _color});

  add_batch(offset, queue::command::type::k_point, _flags);
}

void immediate3D::generate_line(const math::vec3f& _point_a,
  const math::vec3f& _point_b, const math::vec4f& _color, rx_u32 _flags)
{
  const auto offset{m_elements.size()};
  const auto element{static_cast<rx_u32>(m_vertices.size())};

  m_elements.push_back(element + 0);
  m_elements.push_back(element + 1);

  m_vertices.push_back({_point_a, 0.0f, _color});
  m_vertices.push_back({_point_b, 0.0f, _color});

  add_batch(offset, queue::command::type::k_line, _flags);
}

void immediate3D::generate_solid_sphere(const math::vec2f& _slices_and_stacks,
  const math::mat4x4f& _transform, const math::vec4f& _color, rx_u32 _flags)
{
  const math::vec2f begin{};
  const math::vec2f end{math::k_pi<rx_f32> * 2.0f, math::k_pi<rx_f32>};
  const math::vec2f step{(end - begin) / _slices_and_stacks};

  auto parametric{[](const math::vec2f& _uv) -> math::vec3f {
    const auto cos_x{math::cos(_uv.x)};
    const auto cos_y{math::cos(_uv.y)};
    const auto sin_x{math::sin(_uv.x)};
    const auto sin_y{math::sin(_uv.y)};
    return {cos_x * sin_y, cos_y, sin_x * sin_y};
  }};

  const auto offset{m_elements.size()};
  for (rx_f32 i{0.0f}; i < _slices_and_stacks.x; i++) {
    for (rx_f32 j{0.0f}; j < _slices_and_stacks.y; j++) {
      const rx_f32 ua{i * step.x + begin.x};
      const rx_f32 va{j * step.y + begin.y};
      const rx_f32 ub{i + 1.0f == _slices_and_stacks.x ? end.x : (i + 1.0f) * step.x + begin.x};
      const rx_f32 vb{j + 1.0f == _slices_and_stacks.y ? end.y : (j + 1.0f) * step.y + begin.y};

      const math::vec3f& a{math::mat4x4f::transform_point(parametric({ua, va}), _transform)};
      const math::vec3f& b{math::mat4x4f::transform_point(parametric({ua, vb}), _transform)};
      const math::vec3f& c{math::mat4x4f::transform_point(parametric({ub, va}), _transform)};
      const math::vec3f& d{math::mat4x4f::transform_point(parametric({ub, vb}), _transform)};

      const auto element{static_cast<rx_u32>(m_vertices.size())};

      m_elements.push_back(element + 0); // a
      m_elements.push_back(element + 2); // c
      m_elements.push_back(element + 1); // b
      m_elements.push_back(element + 3); // d
      m_elements.push_back(element + 1); // b
      m_elements.push_back(element + 2); // c
    
      m_vertices.push_back({a, 0.0f, _color});
      m_vertices.push_back({b, 0.0f, _color});
      m_vertices.push_back({c, 0.0f, _color});
      m_vertices.push_back({d, 0.0f, _color});
    }
  }

  return add_batch(offset, queue::command::type::k_solid_sphere, _flags);
}

void immediate3D::add_batch(rx_size _offset, queue::command::type _type,
  rx_u32 _flags)
{
  const rx_size count{m_elements.size() - _offset};

  frontend::state render_state;

  // disable blending
  render_state.blend.record_enable(false);

  // alpha blending
  render_state.blend.record_blend_factors(
    frontend::blend_state::factor_type::k_src_alpha,
    frontend::blend_state::factor_type::k_one_minus_src_alpha);

  // determing depth state from flags
  render_state.depth.record_test(!!(_flags & k_depth_test));
  render_state.depth.record_write(!!(_flags & k_depth_write));

  // backface culling
  render_state.cull.record_enable(true);

  // calculate final state
  render_state.flush();

  // coalesce this batch if at all possible
  if (!m_batches.is_empty()) {
    auto& batch{m_batches.last()};
    if (batch.kind == _type && batch.render_state == render_state) {
      batch.count += count;
      return;
    }
  }

  m_batches.push_back({_offset, count, _type, render_state});
}

} // namespace rx::render