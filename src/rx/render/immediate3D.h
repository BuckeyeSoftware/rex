#ifndef RX_RENDER_IMMEDIATE3D_H
#define RX_RENDER_IMMEDIATE3D_H
#include "rx/core/vector.h"

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
        WIRE_SPHERE,
        WIRE_BOX
      };

      Type kind;
      Uint32 flags;
      Math::Vec4f color;

      union {
        Utility::Nat as_nat;
        Point as_point;
        Line as_line;
        SolidSphere as_solid_sphere;
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
    bool record_wire_sphere(const Math::Vec2f& _slices_and_stack,
      const Math::Vec4f& _color, const Math::Mat4x4f& _transform, Uint8 _flags);
    bool record_wire_box(const Math::Vec4f& _color, const Math::AABB& _aabb,
      Uint8 _flags);

    bool is_empty() const;

  private:
    friend struct Immediate3D;
    Vector<Command> m_commands;
  };

  Immediate3D(Frontend::Context* _frontend);
  ~Immediate3D();

  void render(Frontend::Target* _target, const Math::Mat4x4f& _view,
              const Math::Mat4x4f& _projection);

  Queue& frame_queue();

private:
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

  static constexpr const Size BUFFERS = 2;

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
  Vector<Batch> m_render_batches[BUFFERS];
  Frontend::Buffer* m_buffers[BUFFERS];
  Queue m_render_queue[BUFFERS];
};

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

} // namespace Rx::Render

#endif // RX_RENDER_IMMEDIATE3D_H
