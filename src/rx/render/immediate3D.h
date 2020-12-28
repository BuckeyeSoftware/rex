#ifndef RX_RENDER_IMMEDIATE3D_H
#define RX_RENDER_IMMEDIATE3D_H
#include "rx/core/vector.h"

#include "rx/math/vec3.h"
#include "rx/math/mat4x4.h"

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
    k_depth_test  = 1 << 0,
    k_depth_write = 1 << 1
  };

  struct Queue {
    RX_MARK_NO_COPY(Queue);

    Queue() = default;
    Queue(Memory::Allocator& _allocator);
    Queue(Queue&& queue_);

    Queue& operator=(Queue&& queue_);
    bool operator!=(const Queue& _queue) const;

    void clear();

    struct Point {
      Math::Vec3f position;
      Float32 size;
      bool operator!=(const Point& _point) const;
    };

    struct Line {
      Math::Vec3f point_a;
      Math::Vec3f point_b;
      bool operator!=(const Line& _line) const;
    };

    struct SolidSphere {
      Math::Vec2f slices_and_stacks;
      Math::Mat4x4f transform;
      bool operator!=(const SolidSphere& _solid_sphere) const;
    };

    struct SolidCube {
      Math::Mat4x4f transform;
      bool operator!=(const SolidCube& _solid_cube) const;
    };

    struct Command {
      constexpr Command();

      enum class Type {
        k_uninitialized,
        k_point,
        k_line,
        k_solid_sphere,
        k_solid_cube
      };

      bool operator!=(const Command& _command) const;

      Type kind;
      Uint32 flags;
      Size hash;
      Math::Vec4f color;

      union {
        Utility::Nat as_nat;
        Point as_point;
        Line as_line;
        SolidSphere as_solid_sphere;
        SolidCube as_solid_cube;
      };
    };

    bool record_point(const Math::Vec3f& _point, const Math::Vec4f& _color,
                      Float32 _size, Uint8 _flags);

    bool record_line(const Math::Vec3f& _point_a, const Math::Vec3f& _point_b,
                     const Math::Vec4f& _color, Uint8 _flags);

    bool record_solid_sphere(const Math::Vec2f& _slices_and_stacks,
                             const Math::Vec4f& _color, const Math::Mat4x4f& _transform, Uint8 _flags);

    bool record_solid_cube(const Math::Vec4f& _color,
                           const Math::Mat4x4f& _transform, Uint8 _flags);

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
    Float32 size;
    Math::Vec4f color;
  };

  struct Batch {
    Size count;
    Size offset;
    Queue::Command::Type type;
    Frontend::State render_state;
  };

  void generate_point(const Math::Vec3f& _position, Float32 _size,
                      const Math::Vec4f& _color, Uint32 _flags);

  void generate_line(const Math::Vec3f& _point_a, const Math::Vec3f& _point_b,
                     const Math::Vec4f& _color, Uint32 _flags);

  void generate_solid_sphere(const Math::Vec2f& _slices_and_stacks,
                             const Math::Mat4x4f& _transform,
                             const Math::Vec4f& _color, Uint32 _flags);

  void generate_solid_cube(const Math::Mat4x4f& _transform,
                           const Math::Vec4f& _color, Uint32 _flags);

  void size_point(Size& n_vertices_, Size& n_elements_);
  void size_line(Size& n_vertices_, Size& n_elements_);
  void size_solid_sphere(const Math::Vec2f& _slices_and_stacks,
    Size& n_vertices_, Size& n_elements_);
  void size_solid_cube(Size& n_vertices_, Size& n_elements_);

  bool add_batch(Size _offset, Queue::Command::Type _type, Uint32 _flags,
                 bool _blend);

  void add_element(Uint32 _element);
  void add_vertex(Vertex&& vertex_);

  static constexpr const Size k_buffers{2};

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;

  Queue m_queue;
  Vertex* m_vertices;
  Uint32* m_elements;
  Vector<Batch> m_batches;

  Size m_vertex_index;
  Size m_element_index;

  Size m_rd_index;
  Size m_wr_index;
  Vector<Batch> m_render_batches[k_buffers];
  Frontend::Buffer* m_buffers[k_buffers];
  Queue m_render_queue[k_buffers];
};

inline bool Immediate3D::Queue::Point::operator!=(const Point& _point) const {
  return _point.position != position || _point.size != size;
}

inline bool Immediate3D::Queue::Line::operator!=(const Line& _line) const {
  return _line.point_a != point_a || _line.point_b != point_b;
}

inline bool Immediate3D::Queue::SolidSphere::operator!=(const SolidSphere& _solid_sphere) const {
  return _solid_sphere.slices_and_stacks != slices_and_stacks || _solid_sphere.transform != transform;
}

inline bool Immediate3D::Queue::SolidCube::operator!=(const SolidCube& _solid_cube) const {
  return _solid_cube.transform != transform;
}

inline constexpr Immediate3D::Queue::Command::Command()
  : kind{Queue::Command::Type::k_uninitialized}
  , flags{0}
  , hash{0}
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
