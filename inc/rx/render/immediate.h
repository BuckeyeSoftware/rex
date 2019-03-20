#ifndef RX_RENDER_IMMEDIATE_H
#define RX_RENDER_IMMEDIATE_H

#include <rx/math/vec2.h>
#include <rx/math/vec4.h>

#include <rx/core/utility/nat.h>
#include <rx/core/string.h>

#include <rx/render/target.h>
#include <rx/render/buffer.h>
#include <rx/render/state.h>
#include <rx/render/technique.h>

namespace rx::render {

struct immediate {
  struct queue {
    struct box {
      math::vec2i position;
      math::vec2i size;
      bool operator!=(const box& _box) const;
    };

    struct rectangle : box {
      rx_s32 roundness;
      bool operator!=(const rectangle& _rectangle) const;
    };

    struct triangle : box {};
    struct scissor : box {};

    struct line {
      math::vec2i points[2];
      rx_s32 roundness;
      rx_s32 thickness;
      bool operator!=(const line& _line) const;
    };

    struct command {
      command() : as_nat{} {}

      enum class category {
        k_rectangle,
        k_triangle,
        k_line,
        k_scissor
      };

      bool operator!=(const command& _command) const;

      category type;
      rx_u32 flags;
      rx_size hash;
      math::vec4f color;

      union {
        utility::nat as_nat;
        line as_line;
        rectangle as_rectangle;
        scissor as_scissor;
        triangle as_triangle;
      };
    };

    void record_scissor(const math::vec2i& _position, const math::vec2i& _size);
    void record_rectangle(const math::vec2i& _position, const math::vec2i& _size, rx_s32 _roundness, const math::vec4f& _color);
    void record_line(const math::vec2i& _point_a, const math::vec2i& _point_b, rx_s32 _roundness, rx_s32 _thickness, const math::vec4f& _color);
    void record_triangle(const math::vec2i& _position, const math::vec2i& _size, rx_u32 _flags, const math::vec4f& _color);

    bool operator!=(const queue& _queue) const;
    void clear();

    bool is_empty() const {
      return m_commands.is_empty();
    }
    
    array<command> m_commands;
  };

  immediate(frontend* _frontend, technique* _technique);
  ~immediate();

  void render(target* _target);

  operator queue&();

private:
  static constexpr const rx_size k_buffers{2};
  static constexpr const rx_size k_circle_vertices{16*4};

  template<rx_size E>
  void generate_polygon(const math::vec2f (&coordinates)[E],
    rx_f32 _thickness, const math::vec4f& _color, queue::command::category _from_type);

  void generate_rectangle(const math::vec2f& _position, const math::vec2f& _size,
    rx_f32 _roundness, const math::vec4f& _color);

  void generate_line(const math::vec2f& _point_a,
    const math::vec2f& _point_b, rx_f32 _thickness, rx_f32 _roundness,
    const math::vec4f& _color);

  void add_batch(rx_size _offset, queue::command::category _category);

  struct vertex {
    math::vec2f position;
    math::vec2f coordinate;
    math::vec4f color;
  };

  struct batch {
    rx_size offset;
    rx_size count;
    queue::command::category category;
    state render_state;
  };

  frontend* m_frontend;
  technique* m_technique;

  // current scissor rectangle
  math::vec2i m_scissor_position;
  math::vec2i m_scissor_size;

  // precomputed circle vertices
  math::vec2f m_circle_vertices[k_circle_vertices];

  // generated commands, vertices, elements and batches
  queue m_queue;
  array<vertex> m_vertices;
  array<rx_u32> m_elements;
  array<batch> m_batches;

  // buffering of batched immediates
  rx_size m_rd_index;
  rx_size m_wr_index;
  array<batch> m_render_batches[k_buffers];
  buffer* m_buffers[k_buffers];
  queue m_render_queue[k_buffers];
};

inline bool immediate::queue::box::operator!=(const box& _box) const {
  return _box.position != position || _box.size == size;
}

inline bool immediate::queue::rectangle::operator!=(const rectangle& _rectangle) const {
  return box::operator!=(_rectangle) || _rectangle.roundness != roundness;
}

inline bool immediate::queue::line::operator!=(const line& _line) const {
  return _line.points[0] != points[0] || _line.points[1] != points[1] ||
    _line.roundness != roundness || _line.thickness != thickness;
}

inline immediate::operator queue&() {
  return m_queue;
}

} // namespace rx::render

#endif // RX_RENDER_IMMEDIATE_H