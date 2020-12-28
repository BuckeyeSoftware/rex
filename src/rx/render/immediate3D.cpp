#include <stddef.h> // offsetof

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/render/immediate3D.h"

#include "rx/core/profiler.h"

namespace Rx::Render {

Immediate3D::Queue::Queue(Memory::Allocator& _allocator)
  : m_commands{_allocator}
{
}

Immediate3D::Queue::Queue(Queue&& queue_)
  : m_commands{Utility::move(queue_.m_commands)}
{
}

Immediate3D::Queue& Immediate3D::Queue::operator=(Queue&& queue_) {
  RX_ASSERT(&queue_ != this, "self assignment");
  m_commands = Utility::move(queue_.m_commands);
  return *this;
}

bool Immediate3D::Queue::Command::operator!=(const Command& _command) const {
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
  case Command::Type::k_uninitialized:
    return false;
  case Command::Type::k_point:
    return _command.as_point != as_point;
  case Command::Type::k_line:
    return _command.as_line != as_line;
  case Command::Type::k_solid_sphere:
    return _command.as_solid_sphere != as_solid_sphere;
  case Command::Type::k_solid_cube:
    return _command.as_solid_cube != as_solid_cube;
  }

  return false;
}

bool Immediate3D::Queue::record_point(const Math::Vec3f& _point,
                                      const Math::Vec4f& _color, Float32 _size, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::k_point;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_point.position = _point;
  next_command.as_point.size = _size;

  next_command.hash = Hash<Uint32>{}(static_cast<Uint32>(next_command.kind));
  next_command.hash = Hash<Uint32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec3f>{}(next_command.as_point.position));
  next_command.hash = hash_combine(next_command.hash, Hash<Float32>{}(next_command.as_point.size));

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::record_line(const Math::Vec3f& _point_a,
                                     const Math::Vec3f& _point_b, const Math::Vec4f& _color, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::k_line;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_line.point_a = _point_a;
  next_command.as_line.point_b = _point_b;

  next_command.hash = Hash<Uint32>{}(static_cast<Uint32>(next_command.kind));
  next_command.hash = Hash<Uint32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec3f>{}(next_command.as_line.point_a));
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec3f>{}(next_command.as_line.point_b));

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::record_solid_sphere(const Math::Vec2f& _slices_and_stacks,
                                             const Math::Vec4f& _color, const Math::Mat4x4f& _transform, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::k_solid_sphere;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_solid_sphere.slices_and_stacks = _slices_and_stacks;
  next_command.as_solid_sphere.transform = _transform;

  next_command.hash = Hash<Uint32>{}(static_cast<Uint32>(next_command.kind));
  next_command.hash = Hash<Uint32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec2f>{}(next_command.as_solid_sphere.slices_and_stacks));
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Mat4x4f>{}(next_command.as_solid_sphere.transform));

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::record_solid_cube(const Math::Vec4f& _color,
                                           const Math::Mat4x4f& _transform, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::k_solid_cube;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_solid_cube.transform = _transform;

  next_command.hash = Hash<Uint32>{}(static_cast<Uint32>(next_command.kind));
  next_command.hash = Hash<Uint32>{}(next_command.flags);
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Vec4f>{}(next_command.color));
  next_command.hash = hash_combine(next_command.hash, Hash<Math::Mat4x4f>{}(next_command.as_solid_cube.transform));

  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::operator!=(const Queue& _queue) const {
  if (_queue.m_commands.size() != m_commands.size()) {
    return true;
  }

  for (Size i{0}; i < m_commands.size(); i++) {
    if (_queue.m_commands[i] != m_commands[i]) {
      return true;
    }
  }

  return false;
}

void Immediate3D::Queue::clear() {
  m_commands.clear();
}

Immediate3D::Immediate3D(Frontend::Context* _frontend)
  : m_frontend{_frontend}
  , m_technique{m_frontend->find_technique_by_name("immediate3D")}
  , m_queue{m_frontend->allocator()}
  , m_vertices{nullptr}
  , m_elements{nullptr}
  , m_batches{m_frontend->allocator()}
  , m_vertex_index{0}
  , m_element_index{0}
  , m_rd_index{1}
  , m_wr_index{0}
{
  for (Size i{0}; i < k_buffers; i++) {
    m_render_batches[i] = {m_frontend->allocator()};
    m_render_queue[i] = {m_frontend->allocator()};
  }

  Frontend::Buffer::Format format;
  format.record_type(Frontend::Buffer::Type::DYNAMIC);
  format.record_element_type(Frontend::Buffer::ElementType::U32);
  format.record_vertex_stride(sizeof(Vertex));
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::VEC3F, offsetof(Vertex, position)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32, offsetof(Vertex, size)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::VEC3F, offsetof(Vertex, color)});
  format.finalize();

  for (Size i{0}; i < k_buffers; i++) {
    m_buffers[i] = m_frontend->create_buffer(RX_RENDER_TAG("immediate3D"));
    m_buffers[i]->record_format(format);
    m_frontend->initialize_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[i]);
  }
}

Immediate3D::~Immediate3D() {
  for(Size i{0}; i < k_buffers; i++) {
    m_frontend->destroy_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[i]);
  }
}

void Immediate3D::render(Frontend::Target* _target, const Math::Mat4x4f& _view,
                         const Math::Mat4x4f& _projection)
{
  RX_PROFILE_CPU("immediate3D::render");

  // avoid rendering if the last update did not produce any draw commands and
  // this iteration has no updates either
  const bool last_empty = m_render_queue[m_rd_index].is_empty();
  if (last_empty && m_queue.is_empty()) {
    return;
  }

  // avoid generating geomtry and uploading if the contents didn't change
  if (m_queue != m_render_queue[m_rd_index]) {
    // calculate storage needed
    Size n_vertices = 0;
    Size n_elements = 0;
    m_queue.m_commands.each_fwd([&](const Queue::Command& _command) {
      switch (_command.kind) {
      case Queue::Command::Type::k_point:
        size_point(n_vertices, n_elements);
        break;
      case Queue::Command::Type::k_line:
        size_line(n_vertices, n_elements);
        break;
      case Queue::Command::Type::k_solid_sphere:
        size_solid_sphere(
          _command.as_solid_sphere.slices_and_stacks,
          n_vertices,
          n_elements);
        break;
      case Queue::Command::Type::k_solid_cube:
        size_solid_cube(n_vertices, n_elements);
        break;
      default:
        break;
      }
    });

    // The commands generated did not produce any primitives to render.
    if (n_elements == 0) {
      return;
    }

    // allocate storage
    m_vertices = (Vertex*)m_buffers[m_wr_index]->map_vertices(n_vertices * sizeof(Vertex));
    m_elements = (Uint32*)m_buffers[m_wr_index]->map_elements(n_elements * sizeof(Uint32));

    // generate geometry for a future frame
    m_queue.m_commands.each_fwd([this](const Queue::Command& _command) {
      switch (_command.kind) {
      case Queue::Command::Type::k_point:
        generate_point(
          _command.as_point.position,
          _command.as_point.size,
          _command.color,
          _command.flags);
        break;
      case Queue::Command::Type::k_line:
        generate_line(
          _command.as_line.point_a,
          _command.as_line.point_b,
          _command.color,
          _command.flags);
        break;
      case Queue::Command::Type::k_solid_sphere:
        generate_solid_sphere(
          _command.as_solid_sphere.slices_and_stacks,
          _command.as_solid_sphere.transform,
          _command.color,
          _command.flags);
        break;
      case Queue::Command::Type::k_solid_cube:
        generate_solid_cube(
          _command.as_solid_cube.transform,
          _command.color,
          _command.flags);
      default:
        break;
      }
    });

    // Record the edit.
    m_buffers[m_wr_index]->record_vertices_edit(0, n_vertices * sizeof(Vertex));
    m_buffers[m_wr_index]->record_elements_edit(0, n_elements * sizeof(Uint32));
    m_frontend->update_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[m_wr_index]);

    // Clear staging buffers
    m_vertices = nullptr;
    m_elements = nullptr;

    // Reset indices
    m_vertex_index = 0;
    m_element_index = 0;

    // Write buffer will be processed some time in the future
    m_render_batches[m_wr_index] = Utility::move(m_batches);
    m_render_queue[m_wr_index] = Utility::move(m_queue);

    m_wr_index = (m_wr_index + 1) % k_buffers;
  }

  // if the last queue has any draw commands, render them now
  if (!last_empty) {
    for (Size i = 0; i < 2; i++) {
      m_technique->variant(i)->uniforms()[0].record_mat4x4f(_view);
      m_technique->variant(i)->uniforms()[1].record_mat4x4f(_projection);
    }

    // process the batches
    m_render_batches[m_rd_index].each_fwd([&](Batch& _batch) {
      _batch.render_state.viewport.record_dimensions(_target->dimensions());

      Frontend::Buffers draw_buffers;
      draw_buffers.add(0);

      switch (_batch.type) {
      case Queue::Command::Type::k_uninitialized:
        RX_HINT_UNREACHABLE();
        break;
      case Queue::Command::Type::k_point:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D points"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(0),
          _batch.count,
          _batch.offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::POINTS,
          {});
        break;
      case Queue::Command::Type::k_line:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D lines"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(1),
          _batch.count,
          _batch.offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::LINES,
          {});
        break;
      case Queue::Command::Type::k_solid_sphere:
        [[fallthrough]];
      case Queue::Command::Type::k_solid_cube:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D triangles"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(1),
          _batch.count,
          _batch.offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::TRIANGLES,
          {});
        break;
      }
    });
    m_rd_index = (m_rd_index + 1) % k_buffers;
  }

  // clear the queue for the next frame
  m_queue.clear();
}

void Immediate3D::generate_point(const Math::Vec3f& _position, Float32 _size,
                                 const Math::Vec4f& _color, Uint32 _flags)
{
  const auto offset = m_element_index;
  const auto element = static_cast<Uint32>(m_vertex_index);

  add_element(element);
  add_vertex({_position, _size, _color});

  add_batch(offset, Queue::Command::Type::k_point, _flags, _color.a < 1.0f);
}

void Immediate3D::generate_line(const Math::Vec3f& _point_a,
                                const Math::Vec3f& _point_b, const Math::Vec4f& _color, Uint32 _flags)
{
  const auto offset = m_element_index;
  const auto element = static_cast<Uint32>(m_vertex_index);

  add_element(element + 0);
  add_element(element + 1);

  add_vertex({_point_a, 0.0f, _color});
  add_vertex({_point_b, 0.0f, _color});

  add_batch(offset, Queue::Command::Type::k_line, _flags, _color.a < 1.0f);
}

void Immediate3D::generate_solid_sphere(const Math::Vec2f& _slices_and_stacks,
                                        const Math::Mat4x4f& _transform, const Math::Vec4f& _color, Uint32 _flags)
{
  const Math::Vec2f begin{};
  const Math::Vec2f end{Math::k_pi<Float32> * 2.0f, Math::k_pi<Float32>};
  const Math::Vec2f step{(end - begin) / _slices_and_stacks};

  auto parametric{[](const Math::Vec2f& _uv) -> Math::Vec3f {
    const auto cos_x{Math::cos(_uv.x)};
    const auto cos_y{Math::cos(_uv.y)};
    const auto sin_x{Math::sin(_uv.x)};
    const auto sin_y{Math::sin(_uv.y)};
    return {cos_x * sin_y, cos_y, sin_x * sin_y};
  }};

  const auto offset = m_element_index;

  for (Float32 i{0.0f}; i < _slices_and_stacks.x; i++) {
    for (Float32 j{0.0f}; j < _slices_and_stacks.y; j++) {
      const Float32 ua{i * step.x + begin.x};
      const Float32 va{j * step.y + begin.y};
      const Float32 ub{i + 1.0f == _slices_and_stacks.x ? end.x : (i + 1.0f) * step.x + begin.x};
      const Float32 vb{j + 1.0f == _slices_and_stacks.y ? end.y : (j + 1.0f) * step.y + begin.y};

      const Math::Vec3f& a{Math::Mat4x4f::transform_point(parametric({ua, va}), _transform)};
      const Math::Vec3f& b{Math::Mat4x4f::transform_point(parametric({ua, vb}), _transform)};
      const Math::Vec3f& c{Math::Mat4x4f::transform_point(parametric({ub, va}), _transform)};
      const Math::Vec3f& d{Math::Mat4x4f::transform_point(parametric({ub, vb}), _transform)};

      const auto element{static_cast<Uint32>(m_vertex_index)};

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

  add_batch(offset, Queue::Command::Type::k_solid_sphere, _flags, _color.a < 1.0f);
}

void Immediate3D::generate_solid_cube(const Math::Mat4x4f& _transform,
                                      const Math::Vec4f& _color, Uint32 _flags)
{
  const Float32 min[]{-1.0f, -1.0f, -1.0f};
  const Float32 max[]{1.0f, 1.0f, 1.0f};

  auto offset = m_element_index;
  auto element = static_cast<Uint32>(m_vertex_index);

  auto face{[&, this](const Math::Vec3f& _a, const Math::Vec3f& _b,
                      const Math::Vec3f& _c, const Math::Vec3f& _d)
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
  const Math::Vec3f t1{Math::Mat4x4f::transform_point({min[0], max[1], min[2]}, _transform)};
  const Math::Vec3f t2{Math::Mat4x4f::transform_point({max[0], max[1], min[2]}, _transform)};
  const Math::Vec3f t3{Math::Mat4x4f::transform_point({max[0], max[1], max[2]}, _transform)};
  const Math::Vec3f t4{Math::Mat4x4f::transform_point({min[0], max[1], max[2]}, _transform)};
  face(t1, t2, t3, t4);

  // Front!
  const Math::Vec3f f1{Math::Mat4x4f::transform_point({min[0], max[1], max[2]}, _transform)};
  const Math::Vec3f f2{Math::Mat4x4f::transform_point({max[0], max[1], max[2]}, _transform)};
  const Math::Vec3f f3{Math::Mat4x4f::transform_point({max[0], min[1], max[2]}, _transform)};
  const Math::Vec3f f4{Math::Mat4x4f::transform_point({min[0], min[1], max[2]}, _transform)};
  face(f1, f2, f3, f4);

  // Left
  const Math::Vec3f l1{Math::Mat4x4f::transform_point({min[0], max[1], max[2]}, _transform)};
  const Math::Vec3f l2{Math::Mat4x4f::transform_point({min[0], min[1], max[2]}, _transform)};
  const Math::Vec3f l3{Math::Mat4x4f::transform_point({min[0], min[1], min[2]}, _transform)};
  const Math::Vec3f l4{Math::Mat4x4f::transform_point({min[0], max[1], min[2]}, _transform)};
  face(l1, l2, l3, l4);

  // Bottom!
  const Math::Vec3f b1{Math::Mat4x4f::transform_point({min[0], min[1], min[2]}, _transform)};
  const Math::Vec3f b2{Math::Mat4x4f::transform_point({min[0], min[1], max[2]}, _transform)};
  const Math::Vec3f b3{Math::Mat4x4f::transform_point({max[0], min[1], max[2]}, _transform)};
  const Math::Vec3f b4{Math::Mat4x4f::transform_point({max[0], min[1], min[2]}, _transform)};
  face(b1, b2, b3, b4);

  // Back!
  const Math::Vec3f B1{Math::Mat4x4f::transform_point({min[0], max[1], min[2]}, _transform)};
  const Math::Vec3f B2{Math::Mat4x4f::transform_point({min[0], min[1], min[2]}, _transform)};
  const Math::Vec3f B3{Math::Mat4x4f::transform_point({max[0], min[1], min[2]}, _transform)};
  const Math::Vec3f B4{Math::Mat4x4f::transform_point({max[0], max[1], min[2]}, _transform)};
  face(B1, B2, B3, B4);

  // Right!
  const Math::Vec3f r1{Math::Mat4x4f::transform_point({max[0], max[1], max[2]}, _transform)};
  const Math::Vec3f r2{Math::Mat4x4f::transform_point({max[0], max[1], min[2]}, _transform)};
  const Math::Vec3f r3{Math::Mat4x4f::transform_point({max[0], min[1], min[2]}, _transform)};
  const Math::Vec3f r4{Math::Mat4x4f::transform_point({max[0], min[1], max[2]}, _transform)};
  face(r1, r2, r3, r4);

  add_batch(offset, Queue::Command::Type::k_solid_cube, _flags, _color.a < 1.0f);
}

void Immediate3D::size_point(Size& n_vertices_, Size& n_elements_) {
  n_vertices_ += 1;
  n_elements_ += 1;
}

void Immediate3D::size_line(Size& n_vertices_, Size& n_elements_) {
  n_vertices_ += 2;
  n_elements_ += 2;
}

void Immediate3D::size_solid_sphere(const Math::Vec2f& _slices_and_stacks,
                                    Size& n_vertices_, Size& n_elements_)
{
  n_vertices_ += 4 * static_cast<Size>(_slices_and_stacks.area());
  n_elements_ += 6 * static_cast<Size>(_slices_and_stacks.area());
}

void Immediate3D::size_solid_cube(Size& n_vertices_, Size& n_elements_) {
  n_vertices_ += 4 * 6;
  n_elements_ += 6 * 6;
}

bool Immediate3D::add_batch(Size _offset, Queue::Command::Type _type,
                            Uint32 _flags, bool _blend)
{
  const Size count = m_element_index - _offset;

  if (count == 0) {
    return true;
  }

  Frontend::State render_state;

  if (_blend) {
    render_state.blend.record_enable(true);
    render_state.blend.record_blend_factors(
            Frontend::BlendState::FactorType::k_src_alpha,
            Frontend::BlendState::FactorType::k_one_minus_src_alpha);
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
    auto& batch = m_batches.last();
    if (batch.type == _type && batch.render_state == render_state) {
      batch.count += count;
      return true;
    }
  }

  return m_batches.emplace_back(count, _offset, _type, render_state);
}

void Immediate3D::add_element(Uint32 _element) {
  m_elements[m_element_index++] = _element;
}

void Immediate3D::add_vertex(Vertex&& vertex_) {
  m_vertices[m_vertex_index++] = Utility::move(vertex_);
}

} // namespace Rx::Render
