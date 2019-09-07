#ifndef RX_RENDER_IMMEDIATE3D_H
#define RX_RENDER_IMMEDIATE3D_H
#include "rx/render/frontend/state.h"

#include "rx/core/vector.h"

#include "rx/math/vec3.h"
#include "rx/math/mat4x4.h"

namespace rx::memory {
  struct allocator;
}

namespace rx::render {

namespace frontend {
  struct interface;
  struct technique;
  struct target;
  struct buffer;
}

struct immediate3D {
  enum : rx_u32 {
    k_depth_test  = 1 << 0,
    k_depth_write = 1 << 1
  };

  struct queue 
    : concepts::no_copy
  {
    queue() = default;
    queue(memory::allocator* _allocator);
    queue(queue&& queue_);

    queue& operator=(queue&& queue_);
    bool operator!=(const queue& _queue) const;

    void clear();

    struct point {
      math::vec3f position;
      rx_f32 size;
      bool operator!=(const point& _point) const;
    };

    struct line {
      math::vec3f point_a;
      math::vec3f point_b;
      bool operator!=(const line& _line) const;
    };

    struct solid_sphere {
      math::vec2f slices_and_stacks;
      math::mat4x4f transform;
      bool operator!=(const solid_sphere& _solid_sphere) const;
    };

    struct solid_cube {
      math::mat4x4f transform;
      bool operator!=(const solid_cube& _solid_cube) const;
    };

    struct command {
      constexpr command();

      enum class type {
        k_uninitialized,
        k_point,
        k_line,
        k_solid_sphere,
        k_solid_cube
      };

      bool operator!=(const command& _command) const;

      type kind;
      rx_u32 flags;
      rx_size hash;
      math::vec4f color;

      union {
        utility::nat as_nat;
        point as_point;
        line as_line;
        solid_sphere as_solid_sphere;
        solid_cube as_solid_cube;
      };
    };

    void record_point(const math::vec3f& _point, const math::vec4f& _color,
      rx_f32 _size, rx_u8 _flags);
  
    void record_line(const math::vec3f& _point_a, const math::vec3f& _point_b,
      const math::vec4f& _color, rx_u8 _flags);

    void record_solid_sphere(const math::vec2f& _slices_and_stacks,
      const math::vec4f& _color, const math::mat4x4f& _transform, rx_u8 _flags);

    void record_solid_cube(const math::vec4f& _color,
      const math::mat4x4f& _transform, rx_u8 _flags);

    bool is_empty() const;

  private:
    friend struct immediate3D;
  
    memory::allocator* m_allocator;
    vector<command> m_commands;
  };

  immediate3D(frontend::interface* _frontend);
  ~immediate3D();

  void render(frontend::target* _target, const math::mat4x4f& _view,
    const math::mat4x4f& _projection);

  queue& frame_queue();

private:
  struct vertex {
    math::vec3f position;
    rx_f32 size;
    math::vec4f color;
  };

  struct batch {
    rx_size offset;
    rx_size count;
    queue::command::type kind;
    frontend::state render_state;
  };

  void generate_point(const math::vec3f& _position, rx_f32 _size,
    const math::vec4f& _color, rx_u32 _flags);

  void generate_line(const math::vec3f& _point_a, const math::vec3f& _point_b,
    const math::vec4f& _color, rx_u32 _flags);

  void generate_solid_sphere(const math::vec2f& _slices_and_stacks,
    const math::mat4x4f& _transform, const math::vec4f& _color, rx_u32 _flags);

  void generate_solid_cube(const math::mat4x4f& _transform,
    const math::vec4f& _color, rx_u32 _flags);

  void size_point(rx_size& n_vertices_, rx_size& n_elements_);
  void size_line(rx_size& n_vertices_, rx_size& n_elements_);
  void size_solid_sphere(const math::vec2f& _slices_and_stacks,
    rx_size& n_vertices_, rx_size& n_elements_);
  void size_solid_cube(rx_size& n_vertices_, rx_size& n_elements_);

  void add_batch(rx_size _offset, queue::command::type _type, rx_u32 _flags);

  void add_element(rx_u32 _element);
  void add_vertex(vertex&& _vertex);

  static constexpr const rx_size k_buffers{2};

  frontend::interface* m_frontend;
  frontend::technique* m_technique;

  queue m_queue;
  vector<vertex> m_vertices;
  vector<rx_u32> m_elements;
  vector<batch> m_batches;

  rx_size m_vertex_index;
  rx_size m_element_index;

  rx_size m_rd_index;
  rx_size m_wr_index;
  vector<batch> m_render_batches[k_buffers];
  frontend::buffer* m_buffers[k_buffers];
  queue m_render_queue[k_buffers];
};

inline bool immediate3D::queue::point::operator!=(const point& _point) const {
  return _point.position != position || _point.size != size;
}

inline bool immediate3D::queue::line::operator!=(const line& _line) const {
  return _line.point_a != point_a || _line.point_b != point_b;
}

inline bool immediate3D::queue::solid_sphere::operator!=(const solid_sphere& _solid_sphere) const {
  return _solid_sphere.slices_and_stacks != slices_and_stacks || _solid_sphere.transform != transform;
}

inline bool immediate3D::queue::solid_cube::operator!=(const solid_cube& _solid_cube) const {
  return _solid_cube.transform != transform;
}

inline constexpr immediate3D::queue::command::command()
  : kind{queue::command::type::k_uninitialized}
  , flags{0}
  , hash{0}
  , color{}
  , as_nat{}
{
}

inline bool immediate3D::queue::is_empty() const {
  return m_commands.is_empty();
}

inline immediate3D::queue& immediate3D::frame_queue() {
  return m_queue;
}

} // namespace rx::render

#endif // RX_RENDER_IMMEDIATE3D_H