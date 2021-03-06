#ifndef RX_RENDER_IMMEDIATE3D_H
#define RX_RENDER_IMMEDIATE3D_H
#include "rx/core/vector.h"
#include "rx/core/array.h"

#include "rx/math/mat4x4.h"
#include "rx/math/aabb.h"

#include "rx/render/frontend/state.h"

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Technique;
  struct Target;
  struct Buffer;
}

struct Immediate3D {
  RX_MARK_NO_COPY(Immediate3D);

  enum : Uint32 {
    DEPTH_TEST  = 1 << 0,
    DEPTH_WRITE = 1 << 1
  };

  struct Queue {
    RX_MARK_NO_COPY(Queue);

    Queue() = default;
    Queue(Memory::Allocator& _allocator);
    Queue(Queue&& queue_);

    Queue& operator=(Queue&& queue_);

    void clear();

    struct Point {
      Math::Vec3f position;
      Float32 size;
    };

    struct Line {
      Math::Vec3f point_a;
      Math::Vec3f point_b;
    };

    struct SolidSphere {
      Math::Vec2f slices_and_stacks;
      Math::Mat4x4f transform;
    };

    struct SolidBox {
      Math::Mat4x4f transform;
    };

    struct WireSphere {
      Math::Vec2f slices_and_stacks;
      Math::Mat4x4f transform;
    };

    struct WireBox {
      Math::AABB aabb;
    };

    struct Command {
      constexpr Command();

      enum class Type : Uint8 {
        UNINITIALIZED,
        POINT,
        LINE,
        SOLID_SPHERE,
        SOLID_BOX,
        WIRE_SPHERE,
        WIRE_BOX
      };

      Type kind;
      Uint32 flags;
      Math::Vec4f color;

      union {
        struct {} as_nat;
        Point as_point;
        Line as_line;
        SolidSphere as_solid_sphere;
        SolidBox as_solid_box;
        WireSphere as_wire_sphere;
        WireBox as_wire_box;
      };
    };

    // Non-instanced primitives
    bool record_point(const Math::Vec3f& _point, const Math::Vec4f& _color,
      Float32 _size, Uint8 _flags);
    bool record_line(const Math::Vec3f& _point_a, const Math::Vec3f& _point_b,
      const Math::Vec4f& _color, Uint8 _flags);

    // Instanced primitives.
    bool record_solid_sphere(const Math::Vec2f& _slices_and_stacks,
      const Math::Vec4f& _color, const Math::Mat4x4f& _transform, Uint8 _flags);
    bool record_solid_box(const Math::Vec4f& _color,
      const Math::Mat4x4f& _transform, Uint8 _flags);
    bool record_wire_sphere(const Math::Vec2f& _slices_and_stack,
      const Math::Vec4f& _color, const Math::Mat4x4f& _transform, Uint8 _flags);
    bool record_wire_box(const Math::Vec4f& _color, const Math::AABB& _aabb,
      Uint8 _flags);

    bool is_empty() const;

  private:
    friend struct Immediate3D;
    Vector<Command> m_commands;
  };

  static Optional<Immediate3D> create(Frontend::Context* _frontend);

  Immediate3D();
  Immediate3D(Immediate3D&& immediate3D_);
  ~Immediate3D();

  Immediate3D& operator=(Immediate3D&& Immediate3D_);

  void render(Frontend::Target* _target, const Math::Mat4x4f& _view,
              const Math::Mat4x4f& _projection);

  Queue& frame_queue();

private:
  static constexpr const Size BUFFERS = 2;

  struct Vertex {
    Math::Vec3f position;
    Math::Vec4f color;
    Math::Vec3f normal;
  };

  struct Instance {
    Math::Vec4f color;
    Math::Mat4x4f transform;
  };

  struct Batch {
    Size element_count;
    Size element_offset;
    Size instance_count;
    Size instance_offset;
    Queue::Command::Type type;
    Frontend::State render_state;
  };

  // Non-instanced geometries.
  void generate_point(const Math::Vec3f& _position, Float32 _size,
                      const Math::Vec4f& _color, Uint32 _flags);
  void generate_line(const Math::Vec3f& _point_a, const Math::Vec3f& _point_b,
                     const Math::Vec4f& _color, Uint32 _flags);

  // Instanced geometries.
  void generate_solid_sphere(const Math::Vec2f& _slices_and_stacks,
                             const Math::Mat4x4f& _transform,
                             const Math::Vec4f& _color, Uint32 _flags);
  void generate_solid_box(const Math::Mat4x4f& _transform,
                          const Math::Vec4f& _color, Uint32 _flags);
  void generate_wire_sphere(const Math::Vec2f& _slices_and_stacks,
                            const Math::Mat4x4f& _transform,
                            const Math::Vec4f& _color, Uint32 _flags);
  void generate_wire_box(const Math::AABB& _aabb, const Math::Vec4f& _color,
                         Uint32 _flags);

  struct Storage {
    Size vertices  = 0;
    Size elements  = 0;
    Size instances = 0;
    void operator+=(const Storage& _storage) {
      vertices += _storage.vertices;
      elements += _storage.elements;
      instances += _storage.instances;
    }
  };

  Storage calculate_storage(const Queue::Command& _command) const;

  Frontend::State calculate_state(Uint32 _flags, bool _blend) const;

  bool add_batch(Size _element_offset, Size _instance_offset,
                 Queue::Command::Type _type, Uint32 _flags,
                 const Math::Vec4f& _color);

  void add_element(Uint32 _element);
  void add_vertex(const Math::Vec3f& _position,
    const Math::Vec3f& _normal, const Math::Vec4f& _color);
  void add_instance(const Math::Mat4x4f& _transform, const Math::Vec4f& _color);

  Immediate3D(Frontend::Context* _frontend,
    Frontend::Technique* _technique, Array<Frontend::Buffer*[BUFFERS]>&& buffers_);

  void release();

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;

  Queue m_queue;
  Vertex* m_vertices;
  Uint32* m_elements;
  Instance* m_instances;

  Vector<Batch> m_batches;

  Size m_vertex_index;
  Size m_element_index;
  Size m_instance_index;

  Size m_rd_index;
  Size m_wr_index;

  Array<Vector<Batch>[BUFFERS]> m_render_batches;
  Array<Queue[BUFFERS]> m_render_queues;
  Array<Frontend::Buffer*[BUFFERS]> m_buffers;
};

// [Immediate3D::Queue]
inline constexpr Immediate3D::Queue::Command::Command()
  : kind{Queue::Command::Type::UNINITIALIZED}
  , flags{0}
  , color{}
  , as_nat{}
{
}

inline bool Immediate3D::Queue::is_empty() const {
  return m_commands.is_empty();
}

inline Immediate3D::Queue& Immediate3D::frame_queue() {
  return m_queue;
}

// [Immediate3D]
inline Immediate3D::Immediate3D()
  : Immediate3D{nullptr, nullptr, {}}
{
}

inline Immediate3D::Immediate3D(Immediate3D&& immediate3D_)
  : m_frontend{Utility::exchange(immediate3D_.m_frontend, nullptr)}
  , m_technique{Utility::exchange(immediate3D_.m_technique, nullptr)}
  , m_queue{Utility::move(immediate3D_.m_queue)}
  , m_vertices{Utility::exchange(immediate3D_.m_vertices, nullptr)}
  , m_elements{Utility::exchange(immediate3D_.m_elements, nullptr)}
  , m_instances{Utility::exchange(immediate3D_.m_instances, nullptr)}
  , m_batches{Utility::move(immediate3D_.m_batches)}
  , m_vertex_index{Utility::exchange(immediate3D_.m_vertex_index, 0)}
  , m_element_index{Utility::exchange(immediate3D_.m_element_index, 0)}
  , m_instance_index{Utility::exchange(immediate3D_.m_instance_index, 0)}
  , m_rd_index{Utility::exchange(immediate3D_.m_rd_index, 0)}
  , m_wr_index{Utility::exchange(immediate3D_.m_wr_index, 0)}
  , m_render_batches{Utility::move(immediate3D_.m_render_batches)}
  , m_render_queues{Utility::move(immediate3D_.m_render_queues)}
  , m_buffers{Utility::move(immediate3D_.m_buffers)}
{
}

inline Immediate3D::~Immediate3D() {
  release();
}

inline Immediate3D& Immediate3D::operator=(Immediate3D&& immediate3D_) {
  if (this != &immediate3D_) {
    release();
    m_frontend = Utility::exchange(immediate3D_.m_frontend, nullptr);
    m_technique = Utility::exchange(immediate3D_.m_technique, nullptr);
    m_queue = Utility::move(immediate3D_.m_queue);
    m_vertices = Utility::exchange(immediate3D_.m_vertices, nullptr);
    m_elements = Utility::exchange(immediate3D_.m_elements, nullptr);
    m_instances = Utility::exchange(immediate3D_.m_instances, nullptr);
    m_batches = Utility::move(immediate3D_.m_batches);
    m_vertex_index = Utility::exchange(immediate3D_.m_vertex_index, 0);
    m_element_index = Utility::exchange(immediate3D_.m_element_index, 0);
    m_instance_index = Utility::exchange(immediate3D_.m_instance_index, 0);
    m_rd_index = Utility::exchange(immediate3D_.m_rd_index, 0);
    m_wr_index = Utility::exchange(immediate3D_.m_wr_index, 0);
    m_render_batches = Utility::move(immediate3D_.m_render_batches);
    m_render_queues = Utility::move(immediate3D_.m_render_queues);
    m_buffers = Utility::move(immediate3D_.m_buffers);
  }
  return *this;
}

} // namespace Rx::Render

#endif // RX_RENDER_IMMEDIATE3D_H
