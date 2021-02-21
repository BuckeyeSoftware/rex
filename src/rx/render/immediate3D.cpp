#include <stddef.h> // offsetof

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/technique.h"
#include "rx/render/frontend/target.h"
#include "rx/render/frontend/buffer.h"

#include "rx/render/immediate3D.h"

#include "rx/core/profiler.h"

#include "rx/math/transform.h"

namespace Rx::Render {

// [Immediate3D::Queue]
Immediate3D::Queue::Queue(Memory::Allocator& _allocator)
  : m_commands{_allocator}
{
}

Immediate3D::Queue::Queue(Queue&& queue_)
  : m_commands{Utility::move(queue_.m_commands)}
{
}

Immediate3D::Queue& Immediate3D::Queue::operator=(Queue&& queue_) {
  m_commands = Utility::move(queue_.m_commands);
  return *this;
}

bool Immediate3D::Queue::record_point(const Math::Vec3f& _point,
                                      const Math::Vec4f& _color, Float32 _size, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::POINT;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_point.position = _point;
  next_command.as_point.size = _size;
  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::record_line(const Math::Vec3f& _point_a,
                                     const Math::Vec3f& _point_b, const Math::Vec4f& _color, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::LINE;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_line.point_a = _point_a;
  next_command.as_line.point_b = _point_b;
  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::record_solid_sphere(
  const Math::Vec2f& _slices_and_stacks, const Math::Vec4f& _color,
  const Math::Mat4x4f& _transform, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::SOLID_SPHERE;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_solid_sphere.slices_and_stacks = _slices_and_stacks;
  next_command.as_solid_sphere.transform = _transform;
  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::record_wire_sphere(
  const Math::Vec2f& _slices_and_stacks, const Math::Vec4f& _color,
  const Math::Mat4x4f& _transform, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::WIRE_SPHERE;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_wire_sphere.slices_and_stacks = _slices_and_stacks;
  next_command.as_wire_sphere.transform = _transform;
  return m_commands.push_back(Utility::move(next_command));
}

bool Immediate3D::Queue::record_wire_box(const Math::Vec4f& _color,
  const Math::AABB& _aabb, Uint8 _flags)
{
  Command next_command;
  next_command.kind = Command::Type::WIRE_BOX;
  next_command.flags = _flags;
  next_command.color = _color;
  next_command.as_wire_box.aabb = _aabb;
  return m_commands.push_back(Utility::move(next_command));
}

void Immediate3D::Queue::clear() {
  m_commands.clear();
}

// [Immediate3D]
Optional<Immediate3D> Immediate3D::create(Frontend::Context* _frontend) {
  auto technique = _frontend->find_technique_by_name("immediate3D");
  if (!technique) {
    return nullopt;
  }


  //for (Size i{0}; i < BUFFERS; i++) {
  //  m_render_batches[i] = {m_frontend->allocator()};
  //  m_render_queue[i] = {m_frontend->allocator()};
  //}

  Frontend::Buffer::Format format;
  format.record_type(Frontend::Buffer::Type::DYNAMIC);
  format.record_element_type(Frontend::Buffer::ElementType::U32);
  format.record_vertex_stride(sizeof(Vertex));
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, position)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, normal)});
  format.record_vertex_attribute({Frontend::Buffer::Attribute::Type::F32x3, offsetof(Vertex, color)});
  format.record_instance_stride(sizeof(Instance));
  format.record_instance_attribute({Frontend::Buffer::Attribute::Type::F32x4, offsetof(Instance, color)});
  format.record_instance_attribute({Frontend::Buffer::Attribute::Type::F32x4x4, offsetof(Instance, transform)});
  format.finalize();

  Array<Frontend::Buffer*[BUFFERS]> buffers;
  for (Size i = 0; i < BUFFERS; i++) {
    auto buffer = _frontend->create_buffer(RX_RENDER_TAG("immediate3D"));
    if (buffer) {
      buffer->record_format(format);
      _frontend->initialize_buffer(RX_RENDER_TAG("immediate3D"), buffer);
      buffers[i] = buffer;
    } else {
      for (Size j = 0; j < i; j++) {
        _frontend->destroy_buffer(RX_RENDER_TAG("immediate3D"), buffer);
      }
      return nullopt;
    }
  }

  return Immediate3D{_frontend, technique, Utility::move(buffers)};
}

Immediate3D::Immediate3D(Frontend::Context* _frontend,
  Frontend::Technique* _technique, Array<Frontend::Buffer*[BUFFERS]>&& buffers_)
  : m_frontend{_frontend}
  , m_technique{_technique}
  , m_vertices{nullptr}
  , m_elements{nullptr}
  , m_instances{nullptr}
  , m_vertex_index{0}
  , m_element_index{0}
  , m_instance_index{0}
  , m_rd_index{1}
  , m_wr_index{0}
  , m_buffers{Utility::move(buffers_)}
{
  if (m_frontend) {
    // Wire up allocators.
    auto& allocator = m_frontend->allocator();

    m_queue = {allocator};
    m_batches = {allocator};

    for (Size i = 0; i < BUFFERS; i++) {
      m_render_batches[i] = {allocator};
      m_render_queues[i] = {allocator};
    }
  }
}

void Immediate3D::release() {
  if (m_frontend) {
    for (Size i = 0; i < BUFFERS; i++) {
      m_frontend->destroy_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[i]);
    }
  }
}

void Immediate3D::render(Frontend::Target* _target, const Math::Mat4x4f& _view,
  const Math::Mat4x4f& _projection)
{
  RX_PROFILE_CPU("immediate3D::render");

  // Avoid rendering if the last update did not produce any draw commands and
  // this iteration has no updates either.
  const bool last_empty = m_render_queues[m_rd_index].is_empty();
  if (last_empty && m_queue.is_empty()) {
    return;
  }

  // Calculate storage needed.
  Storage storage;
  m_queue.m_commands.each_fwd([&](const Queue::Command& _command) {
    storage += calculate_storage(_command);
  });

  // The commands generated did not produce any primitives to render.
  if (storage.elements == 0) {
    return;
  }

  // Allocate storage.
  m_vertices  = (Vertex*)m_buffers[m_wr_index]->map_vertices(storage.vertices * sizeof(Vertex));
  m_elements  = (Uint32*)m_buffers[m_wr_index]->map_elements(storage.elements * sizeof(Uint32));
  m_instances = (Instance*)m_buffers[m_wr_index]->map_instances(storage.instances * sizeof(Instance));

  // Generate geometry for a future frame.
  m_queue.m_commands.each_fwd([this](const Queue::Command& _command) {
    switch (_command.kind) {
    case Queue::Command::Type::POINT:
      generate_point(
        _command.as_point.position,
        _command.as_point.size,
        _command.color,
        _command.flags);
      break;
    case Queue::Command::Type::LINE:
      generate_line(
        _command.as_line.point_a,
        _command.as_line.point_b,
        _command.color,
        _command.flags);
      break;
    case Queue::Command::Type::SOLID_SPHERE:
      generate_solid_sphere(
        _command.as_solid_sphere.slices_and_stacks,
        _command.as_solid_sphere.transform,
        _command.color,
        _command.flags);
      break;
    case Queue::Command::Type::WIRE_SPHERE:
      generate_wire_sphere(
        _command.as_solid_sphere.slices_and_stacks,
        _command.as_solid_sphere.transform,
        _command.color,
        _command.flags);
      break;
    case Queue::Command::Type::WIRE_BOX:
      generate_wire_box(
        _command.as_wire_box.aabb,
        _command.color,
        _command.flags);
    default:
      break;
    }
  });

  // Record the edit.
  m_buffers[m_wr_index]->record_vertices_edit(0, storage.vertices * sizeof(Vertex));
  m_buffers[m_wr_index]->record_elements_edit(0, storage.elements * sizeof(Uint32));
  m_buffers[m_wr_index]->record_instances_edit(0, storage.instances * sizeof(Instance));
  m_frontend->update_buffer(RX_RENDER_TAG("immediate3D"), m_buffers[m_wr_index]);

  // Clear staging buffers
  m_vertices = nullptr;
  m_elements = nullptr;
  m_instances = nullptr;

  // Reset indices
  m_vertex_index = 0;
  m_element_index = 0;
  m_instance_index = 0;

  // Write buffer will be processed some time in the future
  m_render_batches[m_wr_index] = Utility::move(m_batches);
  m_render_queues[m_wr_index] = Utility::move(m_queue);

  m_wr_index = (m_wr_index + 1) % BUFFERS;

  // if the last queue has any draw commands, render them now.
  if (!last_empty) {
    for (Size i = 0; i < 3; i++) {
      m_technique->variant(i)->uniforms()[0].record_mat4x4f(_view);
      m_technique->variant(i)->uniforms()[1].record_mat4x4f(_projection);
    }

    // Process the batches.
    m_render_batches[m_rd_index].each_fwd([&](Batch& _batch) {
      _batch.render_state.viewport.record_dimensions(_target->dimensions());

      Frontend::Buffers draw_buffers;
      draw_buffers.add(0);

      switch (_batch.type) {
      case Queue::Command::Type::UNINITIALIZED:
        RX_HINT_UNREACHABLE();
        break;
      case Queue::Command::Type::POINT:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D points"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(0),
          _batch.element_count,
          _batch.element_offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::POINTS,
          {});
        break;
      case Queue::Command::Type::LINE:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D lines"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(1),
          _batch.element_count,
          _batch.element_offset,
          0,
          0,
          0,
          Frontend::PrimitiveType::LINES,
          {});
        break;
      case Queue::Command::Type::WIRE_BOX:
        [[fallthrough]];
      case Queue::Command::Type::WIRE_SPHERE:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D triangles"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(2),
          _batch.element_count,
          _batch.element_offset,
          _batch.instance_count,
          0,
          _batch.instance_offset,
          Frontend::PrimitiveType::LINES,
          {});
        break;
      case Queue::Command::Type::SOLID_SPHERE:
        m_frontend->draw(
          RX_RENDER_TAG("immediate3D triangles"),
          _batch.render_state,
          _target,
          draw_buffers,
          m_buffers[m_rd_index],
          m_technique->variant(2),
          _batch.element_count,
          _batch.element_offset,
          _batch.instance_count,
          0,
          _batch.instance_offset,
          Frontend::PrimitiveType::TRIANGLES,
          {});
        break;
      }
    });
    m_rd_index = (m_rd_index + 1) % BUFFERS;
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

  // Repurpose normal.x for point size when rendering points.
  add_vertex(_position, {_size, 0.0f, 0.0f},_color);

  add_batch(offset, m_instance_index, Queue::Command::Type::POINT, _flags, _color);
}

void Immediate3D::generate_line(const Math::Vec3f& _point_a,
                                const Math::Vec3f& _point_b, const Math::Vec4f& _color, Uint32 _flags)
{
  const auto offset = m_element_index;
  const auto element = static_cast<Uint32>(m_vertex_index);

  add_element(element + 0);
  add_element(element + 1);

  add_vertex(_point_a, {}, _color);
  add_vertex(_point_b, {}, _color);

  add_batch(offset, m_instance_index, Queue::Command::Type::LINE, _flags, _color);
}

void Immediate3D::generate_wire_sphere(const Math::Vec2f& _slices_and_stacks,
  const Math::Mat4x4f& _transform, const Math::Vec4f& _color, Uint32 _flags)
{
  const auto instance_offset = m_instance_index;
  const auto element_offset = m_element_index;

  // Write the instance transform.
  add_instance(_transform, _color);

  // Check if the last batch was an instanced wire sphere.
  auto render_state = calculate_state(_flags, _color.a < 1.0f);
  if (!m_batches.is_empty()) {
    auto& last = m_batches.last();
    if (last.type == Queue::Command::Type::WIRE_SPHERE && last.render_state == render_state) {
      // Extend that batch to include this instanced sphere.
      last.instance_count++;
      return;
    }
  }

  // Otherwise need to create sphere geometry at the origin using the parametric
  // equation. Technically we could avoid generating the same geometry here if
  // we remembered the indices of existing sphere. Revisit this.
  const Math::Vec2f begin;
  const Math::Vec2f end{Math::PI<Float32> * 2.0f, Math::PI<Float32>};
  const Math::Vec2f step = (end - begin) / _slices_and_stacks;

  auto parametric = [](const Math::Vec2f& _uv) -> Math::Vec3f {
    const auto cos_x{Math::cos(_uv.x)};
    const auto cos_y{Math::cos(_uv.y)};
    const auto sin_x{Math::sin(_uv.x)};
    const auto sin_y{Math::sin(_uv.y)};
    return {cos_x * sin_y, cos_y, sin_x * sin_y};
  };

  for (Float32 i{0.0f}; i < _slices_and_stacks.x; i++) {
    for (Float32 j{0.0f}; j < _slices_and_stacks.y; j++) {
      const Float32 ua = i * step.x + begin.x;
      const Float32 va = j * step.y + begin.y;
      const Float32 ub = i + 1.0f == _slices_and_stacks.x ? end.x : (i + 1.0f) * step.x + begin.x;
      const Float32 vb = j + 1.0f == _slices_and_stacks.y ? end.y : (j + 1.0f) * step.y + begin.y;

      const Math::Vec3f& a = parametric({ua, va});
      const Math::Vec3f& b = parametric({ua, vb});
      const Math::Vec3f& c = parametric({ub, va});
      const Math::Vec3f& d = parametric({ub, vb});

      const auto element = static_cast<Uint32>(m_vertex_index);

      add_vertex(a, {}, _color);
      add_vertex(b, {}, _color);
      add_vertex(c, {}, _color);
      add_vertex(d, {}, _color);

      add_element(element + 0 + 0); // a
      add_element(element + 2 + 1); // c
      add_element(element + 2 + 1); // c
      add_element(element + 1 + 2); // b
      add_element(element + 1 + 2); // b
      add_element(element + 0 + 0); // a

      add_element(element + 3 + 0); // d
      add_element(element + 1 + 1); // c
      add_element(element + 1 + 1); // c
      add_element(element + 2 + 2); // b
      add_element(element + 2 + 2); // b
      add_element(element + 3 + 0); // d
    }
  }

  add_batch(element_offset, instance_offset, Queue::Command::Type::WIRE_SPHERE,
    _flags, _color);
}

void Immediate3D::generate_solid_sphere(const Math::Vec2f& _slices_and_stacks,
                                        const Math::Mat4x4f& _transform,
                                        const Math::Vec4f& _color, Uint32 _flags)
{
  const auto instance_offset = m_instance_index;
  const auto element_offset = m_element_index;

  // Write the instance transform.
  add_instance(_transform, _color);

  // Check if the last batch was an instanced sphere.
  auto render_state = calculate_state(_flags, _color.a < 1.0f);
  if (!m_batches.is_empty()) {
    auto& last = m_batches.last();
    if (last.type == Queue::Command::Type::SOLID_SPHERE && last.render_state == render_state) {
      // Extend that batch to include this instanced sphere.
      last.instance_count++;
      return;
    }
  }

  // Otherwise need to create sphere geometry at the origin using the parametric
  // equation. Technically we could avoid generating the same geometry here if
  // we remembered the indices of existing sphere. Revisit this.
  const Math::Vec2f begin;
  const Math::Vec2f end{Math::PI<Float32> * 2.0f, Math::PI<Float32>};
  const Math::Vec2f step = (end - begin) / _slices_and_stacks;

  auto parametric = [](const Math::Vec2f& _uv) -> Math::Vec3f {
    const auto cos_x{Math::cos(_uv.x)};
    const auto cos_y{Math::cos(_uv.y)};
    const auto sin_x{Math::sin(_uv.x)};
    const auto sin_y{Math::sin(_uv.y)};
    return {cos_x * sin_y, cos_y, sin_x * sin_y};
  };

  for (Float32 i{0.0f}; i < _slices_and_stacks.x; i++) {
    for (Float32 j{0.0f}; j < _slices_and_stacks.y; j++) {
      const Float32 ua = i * step.x + begin.x;
      const Float32 va = j * step.y + begin.y;
      const Float32 ub = i + 1.0f == _slices_and_stacks.x ? end.x : (i + 1.0f) * step.x + begin.x;
      const Float32 vb = j + 1.0f == _slices_and_stacks.y ? end.y : (j + 1.0f) * step.y + begin.y;

      const Math::Vec3f& a = parametric({ua, va});
      const Math::Vec3f& b = parametric({ua, vb});
      const Math::Vec3f& c = parametric({ub, va});
      const Math::Vec3f& d = parametric({ub, vb});

      const auto element = static_cast<Uint32>(m_vertex_index);

      add_element(element + 0); // a
      add_element(element + 2); // c
      add_element(element + 1); // b
      add_element(element + 3); // d
      add_element(element + 1); // b
      add_element(element + 2); // c

      // The vertex normal is just the vertex normalized since we're at the origin.
      add_vertex(a, Math::normalize(a), _color);
      add_vertex(b, Math::normalize(b), _color);
      add_vertex(c, Math::normalize(c), _color);
      add_vertex(d, Math::normalize(d), _color);
    }
  }

  add_batch(element_offset, instance_offset, Queue::Command::Type::SOLID_SPHERE,
    _flags, _color);
}

void Immediate3D::generate_wire_box(const Math::AABB& _aabb,
  const Math::Vec4f& _color, Uint32 _flags)
{
  const auto instance_offset = m_instance_index;
  const auto element_offset = m_element_index;

  // Write the instance transform.
  Math::Transform transform;
  transform.scale = _aabb.scale();
  transform.translate = _aabb.origin();
  add_instance(transform.as_mat4(), _color);

  // Check if the last batch was an instanced wire box.
  auto render_state = calculate_state(_flags, _color.a < 1.0f);
  if (!m_batches.is_empty()) {
    auto& last = m_batches.last();
    if (last.type == Queue::Command::Type::WIRE_BOX && last.render_state == render_state) {
      // Extend that batch to include this instanced wire box.
      last.instance_count++;
      return;
    }
  }

  const Math::Vec3f point0{-1.0f, -1.0f, -1.0f};
  const Math::Vec3f point1{ 1.0f,  1.0f,  1.0f};
  const Math::Vec3f point2{point0.x, point0.y, point1.z};
  const Math::Vec3f point3{point0.x, point1.y, point0.z};
  const Math::Vec3f point4{point1.x, point0.y, point0.z};
  const Math::Vec3f point5{point0.x, point1.y, point1.z};
  const Math::Vec3f point6{point1.x, point0.y, point1.z};
  const Math::Vec3f point7{point1.x, point1.y, point0.z};

  const auto element = static_cast<Uint32>(m_vertex_index);

  add_vertex(point0, {}, _color);
  add_vertex(point1, {}, _color);
  add_vertex(point2, {}, _color);
  add_vertex(point3, {}, _color);
  add_vertex(point4, {}, _color);
  add_vertex(point5, {}, _color);
  add_vertex(point6, {}, _color);
  add_vertex(point7, {}, _color);

  add_element(element + 6 - 1); add_element(element + 2 - 1);
  add_element(element + 2 - 1); add_element(element + 8 - 1);
  add_element(element + 8 - 1); add_element(element + 4 - 1);
  add_element(element + 4 - 1); add_element(element + 6 - 1);
  add_element(element + 3 - 1); add_element(element + 7 - 1);
  add_element(element + 7 - 1); add_element(element + 5 - 1);
  add_element(element + 5 - 1); add_element(element + 1 - 1);
  add_element(element + 1 - 1); add_element(element + 3 - 1);
  add_element(element + 6 - 1); add_element(element + 3 - 1);
  add_element(element + 2 - 1); add_element(element + 7 - 1);
  add_element(element + 8 - 1); add_element(element + 5 - 1);
  add_element(element + 4 - 1); add_element(element + 1 - 1);

  add_batch(element_offset, instance_offset, Queue::Command::Type::WIRE_BOX,
    _flags, _color);
}

Immediate3D::Storage Immediate3D::calculate_storage(const Queue::Command& _command) const {
  switch (_command.kind) {
  case Queue::Command::Type::LINE:
    return {2, 2, 0};
  case Queue::Command::Type::POINT:
    return {1, 1, 0};
  case Queue::Command::Type::WIRE_BOX:
    return {8, 24, 1};
  case Queue::Command::Type::SOLID_SPHERE:
    return {
      4 * Size(_command.as_solid_sphere.slices_and_stacks.area()),
      6 * Size(_command.as_solid_sphere.slices_and_stacks.area()),
      1 // Need at least one instance...
    };
  case Queue::Command::Type::WIRE_SPHERE:
    return {
      4 * Size(_command.as_wire_sphere.slices_and_stacks.area()),
      12 * Size(_command.as_wire_sphere.slices_and_stacks.area()),
      1
    };
  case Queue::Command::Type::UNINITIALIZED:
    break;
  }
  return {0, 0, 0};
}

Frontend::State Immediate3D::calculate_state(Uint32 _flags, bool _blend) const {
  Frontend::State render_state;

  if (_blend) {
    render_state.blend.record_enable(true);
    render_state.blend.record_blend_factors(
      Frontend::BlendState::FactorType::SRC_ALPHA,
      Frontend::BlendState::FactorType::ONE_MINUS_SRC_ALPHA);
  } else {
    render_state.blend.record_enable(false);
  }

  // Determing depth state from flags.
  render_state.depth.record_test(!!(_flags & DEPTH_TEST));
  render_state.depth.record_write(!!(_flags & DEPTH_WRITE));

  // Backface culling.
  render_state.cull.record_enable(true);

  // Calculate final state.
  render_state.flush();

  return render_state;
}

bool Immediate3D::add_batch(Size _element_offset, Size _instance_offset,
                            Queue::Command::Type _type, Uint32 _flags,
                            const Math::Vec4f& _color)
{
  const Size element_count = m_element_index - _element_offset;
  const Size instance_count = m_instance_index - _instance_offset;

  // Empty batch.
  if (element_count == 0) {
     return true;
  }

  auto render_state = calculate_state(_flags, _color.a < 1.0f);

  // Coalesce this batch if at all possible.
  if (!m_batches.is_empty() && instance_count == 0) {
    auto& batch = m_batches.last();
    if (batch.type == _type && batch.render_state == render_state) {
      batch.element_count += element_count;
      return true;
    }
  }

  return m_batches.emplace_back(element_count, _element_offset, instance_count,
    _instance_offset, _type, render_state);
}

void Immediate3D::add_element(Uint32 _element) {
  m_elements[m_element_index++] = _element;
}

void Immediate3D::add_vertex(const Math::Vec3f& _position,
  const Math::Vec3f& _normal, const Math::Vec4f& _color)
{
  m_vertices[m_vertex_index++] = { _position, _color, _normal };
}

void Immediate3D::add_instance(const Math::Mat4x4f& _transform, const Math::Vec4f& _color) {
  m_instances[m_instance_index++] = { _color, _transform };
}

} // namespace Rx::Render
