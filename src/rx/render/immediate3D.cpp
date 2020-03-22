#include <stddef.h> // offsetof

#include "rx/render/frontend/interface.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/render/immediate3D.h"

#include "rx/core/profiler.h"

namespace rx::render {

immediate3D::queue::queue(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_commands{m_allocator}
{
}

immediate3D::queue::queue(queue&& queue_)
  : m_allocator{utility::exchange(queue_.m_allocator, nullptr)}
  , m_commands{utility::move(queue_.m_commands)}
{
}

immediate3D::queue& immediate3D::queue::operator=(queue&& queue_) {
  RX_ASSERT(&queue_ != this, "self assignment");
  m_allocator = utility::exchange(queue_.m_allocator, nullptr);
  m_commands = utility::move(queue_.m_commands);
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
  case command::type::k_solid_cube:
    return _command.as_solid_cube != as_solid_cube;
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

void immediate3D::queue::record_solid_cube(const math::vec4f& _color,
  const math::mat4x4f& _transform, rx_u8 _flags)
{
  command next_command;
  next_command.kind = command::type::k_solid_cube;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_solid_cube.transform = _transform;

  next_command.hash = hash<rx_u32>{}(static_cast<rx_u32>(next_command.kind));
  next_command.hash = hash<rx_u32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, hash<math::vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, hash<math::mat4x4f>{}(next_command.as_solid_cube.transform));

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
  , m_vertex_index{0}
  , m_element_index{0}
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
  profiler::cpu_sample sample{"immediate3D::render"};

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
      case queue::command::type::k_point:
        size_point(n_vertices, n_elements);
        break;
      case queue::command::type::k_line:
        size_line(n_vertices, n_elements);
        break;
      case queue::command::type::k_solid_sphere:
        size_solid_sphere(
          _command.as_solid_sphere.slices_and_stacks,
          n_vertices,
          n_elements);
        break;
      case queue::command::type::k_solid_cube:
        size_solid_cube(n_vertices, n_elements);
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
      case queue::command::type::k_solid_cube:
        generate_solid_cube(
          _command.as_solid_cube.transform,
          _command.color,
          _command.flags);
      default:
        break;
      }
    });

    // upload the geometry
    m_buffers[m_wr_index]->write_vertices(m_vertices.data(), m_vertices.size() * sizeof(vertex));
    m_buffers[m_wr_index]->write_elements(m_elements.data(), m_elements.size() * sizeof(rx_u32));

    // Record the edit.
    m_buffers[m_wr_index]->record_vertices_edit(0, m_vertices.size() * sizeof(vertex));
    m_buffers[m_wr_index]->record_elements_edit(0, m_elements.size() * sizeof(rx_u32));
    m_frontend->update_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[m_wr_index]);

    // clear staging buffers
    m_vertices.clear();
    m_elements.clear();

    // reset indices
    m_vertex_index = 0;
    m_element_index = 0;

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
    m_render_batches[m_rd_index].each_fwd([&](batch& _batch) {
      _batch.render_state.viewport.record_dimensions(_target->dimensions());

      frontend::buffers draw_buffers;
      draw_buffers.add(0);

      switch (_batch.kind) {
      case queue::command::type::k_uninitialized:
        RX_HINT_UNREACHABLE();
        break;
      case queue::command::type::k_point:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D points"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(1),
          _batch.count,
          _batch.offset,
          frontend::primitive_type::k_points,
          {});
        break;
      case queue::command::type::k_line:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D lines"),
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
      case queue::command::type::k_solid_sphere:
        [[fallthrough]];
      case queue::command::type::k_solid_cube:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D triangles"),
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
  const auto offset{m_element_index};
  const auto element{static_cast<rx_u32>(m_vertex_index)};

  add_element(element);
  add_vertex({_position, _size, _color});

  add_batch(offset, queue::command::type::k_point, _flags, _color.a < 1.0f);
}

void immediate3D::generate_line(const math::vec3f& _point_a,
  const math::vec3f& _point_b, const math::vec4f& _color, rx_u32 _flags)
{
  const auto offset{m_element_index};
  const auto element{static_cast<rx_u32>(m_vertex_index)};

  add_element(element + 0);
  add_element(element + 1);

  add_vertex({_point_a, 0.0f, _color});
  add_vertex({_point_b, 0.0f, _color});

  add_batch(offset, queue::command::type::k_line, _flags, _color.a < 1.0f);
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

  const auto offset{m_element_index};
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

      const auto element{static_cast<rx_u32>(m_vertex_index)};

      add_element(element + 0); // a
      add_element(element + 2); // c
      add_element(element + 1); // b
      add_element(element + 3); // d
      add_element(element + 1); // b
      add_element(element + 2); // c

      add_vertex({a, 0.0f, _color});
      add_vertex({b, 0.0f, _color});
      add_vertex({c, 0.0f, _color});
      add_vertex({d, 0.0f, _color});
    }
  }

  return add_batch(offset, queue::command::type::k_solid_sphere, _flags, _color.a < 1.0f);
}

void immediate3D::generate_solid_cube(const math::mat4x4f& _transform,
  const math::vec4f& _color, rx_u32 _flags)
{
  const rx_f32 min[]{-1.0f, -1.0f, -1.0f};
  const rx_f32 max[]{ 1.0f,  1.0f,  1.0f};

  auto offset{m_element_index};
  auto element{static_cast<rx_u32>(m_vertex_index)};

  auto face{[&, this](const math::vec3f& _a, const math::vec3f& _b,
                      const math::vec3f& _c, const math::vec3f& _d)
  {
    add_vertex({_a, 0.0f, _color});
    add_vertex({_b, 0.0f, _color});
    add_vertex({_c, 0.0f, _color});
    add_vertex({_d, 0.0f, _color});

    add_element(element + 0);
    add_element(element + 3);
    add_element(element + 2);
    add_element(element + 2);
    add_element(element + 1);
    add_element(element + 0);

    element += 4;
  }};

  // Top!
  const math::vec3f t1{math::mat4x4f::transform_point({min[0], max[1], min[2]}, _transform)};
  const math::vec3f t2{math::mat4x4f::transform_point({max[0], max[1], min[2]}, _transform)};
  const math::vec3f t3{math::mat4x4f::transform_point({max[0], max[1], max[2]}, _transform)};
  const math::vec3f t4{math::mat4x4f::transform_point({min[0], max[1], max[2]}, _transform)};
  face(t1, t2, t3, t4);

  // Front!
  const math::vec3f f1{math::mat4x4f::transform_point({min[0], max[1], max[2]}, _transform)};
  const math::vec3f f2{math::mat4x4f::transform_point({max[0], max[1], max[2]}, _transform)};
  const math::vec3f f3{math::mat4x4f::transform_point({max[0], min[1], max[2]}, _transform)};
  const math::vec3f f4{math::mat4x4f::transform_point({min[0], min[1], max[2]}, _transform)};
  face(f1, f2, f3, f4);

  // Left
  const math::vec3f l1{math::mat4x4f::transform_point({min[0], max[1], max[2]}, _transform)};
  const math::vec3f l2{math::mat4x4f::transform_point({min[0], min[1], max[2]}, _transform)};
  const math::vec3f l3{math::mat4x4f::transform_point({min[0], min[1], min[2]}, _transform)};
  const math::vec3f l4{math::mat4x4f::transform_point({min[0], max[1], min[2]}, _transform)};
  face(l1, l2, l3, l4);

  // Bottom!
  const math::vec3f b1{math::mat4x4f::transform_point({min[0], min[1], min[2]}, _transform)};
  const math::vec3f b2{math::mat4x4f::transform_point({min[0], min[1], max[2]}, _transform)};
  const math::vec3f b3{math::mat4x4f::transform_point({max[0], min[1], max[2]}, _transform)};
  const math::vec3f b4{math::mat4x4f::transform_point({max[0], min[1], min[2]}, _transform)};
  face(b1, b2, b3, b4);

  // Back!
  const math::vec3f B1{math::mat4x4f::transform_point({min[0], max[1], min[2]}, _transform)};
  const math::vec3f B2{math::mat4x4f::transform_point({min[0], min[1], min[2]}, _transform)};
  const math::vec3f B3{math::mat4x4f::transform_point({max[0], min[1], min[2]}, _transform)};
  const math::vec3f B4{math::mat4x4f::transform_point({max[0], max[1], min[2]}, _transform)};
  face(B1, B2, B3, B4);

  // Right!
  const math::vec3f r1{math::mat4x4f::transform_point({max[0], max[1], max[2]}, _transform)};
  const math::vec3f r2{math::mat4x4f::transform_point({max[0], max[1], min[2]}, _transform)};
  const math::vec3f r3{math::mat4x4f::transform_point({max[0], min[1], min[2]}, _transform)};
  const math::vec3f r4{math::mat4x4f::transform_point({max[0], min[1], max[2]}, _transform)};
  face(r1, r2, r3, r4);

  add_batch(offset, queue::command::type::k_solid_cube, _flags, _color.a < 1.0f);
}

void immediate3D::size_point(rx_size& n_vertices_, rx_size& n_elements_) {
  n_vertices_ += 1;
  n_elements_ += 1;
}

void immediate3D::size_line(rx_size& n_vertices_, rx_size& n_elements_) {
  n_vertices_ += 2;
  n_elements_ += 2;
}

void immediate3D::size_solid_sphere(const math::vec2f& _slices_and_stacks,
  rx_size& n_vertices_, rx_size& n_elements_)
{
  n_vertices_ += 4 * _slices_and_stacks.area();
  n_elements_ += 6 * _slices_and_stacks.area();
}

void immediate3D::size_solid_cube(rx_size& n_vertices_, rx_size& n_elements_) {
  n_vertices_ += 4 * 6;
  n_elements_ += 6 * 6;
}

void immediate3D::add_batch(rx_size _offset, queue::command::type _type,
  rx_u32 _flags, bool _blend)
{
  const rx_size count{m_element_index - _offset};

  frontend::state render_state;

  if (_blend) {
    render_state.blend.record_enable(true);
    render_state.blend.record_blend_factors(
      frontend::blend_state::factor_type::k_src_alpha,
      frontend::blend_state::factor_type::k_one_minus_src_alpha);
  } else {
    render_state.blend.record_enable(false);
  }

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

void immediate3D::add_element(rx_u32 _element) {
  m_elements[m_element_index++] = _element;
}

void immediate3D::add_vertex(vertex&& vertex_) {
  m_vertices[m_vertex_index++] = utility::move(vertex_);
}

} // namespace rx::render
