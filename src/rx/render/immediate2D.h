#ifndef RX_RENDER_IMMEDIATE2D_H
#define RX_RENDER_IMMEDIATE2D_H
#include <string.h> // strlen

#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

#include "rx/core/utility/nat.h"
#include "rx/core/string.h"
#include "rx/core/map.h"

#include "rx/render/frontend/state.h"

namespace rx::render {

namespace frontend {
  struct buffer;
  struct target;
  struct technique;
  struct texture2D;
  struct interface;
}

struct immediate2D {
  enum class text_align {
    k_left,
    k_center,
    k_right
  };

  struct queue 
    : concepts::no_copy
  {
    queue() = default;
    queue(memory::allocator* _allocator);
    queue(queue&& _queue);

    queue& operator=(queue&& _queue);

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

    struct text {
      math::vec2i position;
      rx_s32 size;
      rx_f32 scale;
      rx_size font_index;
      rx_size font_length;
      rx_size text_index;
      rx_size text_length;
      bool operator!=(const text& _text) const;
    };

    struct command {
      constexpr command();

      enum class type {
        k_uninitialized,
        k_rectangle,
        k_triangle,
        k_line,
        k_text,
        k_scissor
      };

      bool operator!=(const command& _command) const;

      type kind;
      rx_u32 flags;
      rx_size hash;
      math::vec4f color;

      union {
        utility::nat as_nat;
        line as_line;
        text as_text;
        rectangle as_rectangle;
        scissor as_scissor;
        triangle as_triangle;
      };
    };

    void record_scissor(const math::vec2i& _position, const math::vec2i& _size);
    void record_rectangle(const math::vec2i& _position, const math::vec2i& _size,
      rx_s32 _roundness, const math::vec4f& _color);
    void record_line(const math::vec2i& _point_a, const math::vec2i& _point_b,
      rx_s32 _roundness, rx_s32 _thickness, const math::vec4f& _color);
    void record_triangle(const math::vec2i& _position, const math::vec2i& _size,
      rx_u32 _flags, const math::vec4f& _color);

    void record_text(const char* _font, rx_size _font_length,
      const math::vec2i& _position, rx_s32 _size, rx_f32 _scale, text_align _align,
      const char* _contents, rx_size _contents_length, const math::vec4f& _color);

    void record_text(const char* _font, const math::vec2i& _position,
      rx_s32 _size, rx_f32 _scale, text_align _align, const char* _contents,
      const math::vec4f& _color);

    void record_text(const string& _font, const math::vec2i& _position,
      rx_s32 _size, rx_f32 _scale, text_align _align, const string& _contents,
      const math::vec4f& _color);

    bool operator!=(const queue& _queue) const;
    void clear();
    bool is_empty() const;

  private:
    friend struct immediate2D;

    memory::allocator* m_allocator;
    array<command> m_commands;
    array<char> m_string_table;
  };

  immediate2D(frontend::interface* _frontend);
  ~immediate2D();

  void render(frontend::target* _target);
  queue& frame_queue();

  struct font {
    static constexpr const rx_size k_default_resolution{128};

    struct quad {
      math::vec2f position[2];
      math::vec2f coordinate[2];
    };

    struct glyph {
      math::vec2<rx_u16> position[2];
      math::vec2f offset;
      rx_f32 x_advance;
    };

    struct key {
      rx_s32 size;
      string name;
      rx_size hash() const;
      bool operator==(const key& _key) const;
    };

    font(const key& _key, frontend::interface* _frontend);
    font(font&& _font);
    ~font();

    quad quad_for_glyph(rx_size _glyph, rx_f32 _scale, math::vec2f& position_) const;
    glyph glyph_for_code(rx_u32 _code) const;

    rx_s32 size() const;
    frontend::texture2D* texture() const;
  
  private:
    frontend::interface* m_frontend;
    rx_s32 m_size;
    rx_size m_resolution;
    frontend::texture2D* m_texture;
    array<glyph> m_glyphs;
  };

private:
  static constexpr const rx_size k_buffers{2};
  static constexpr const rx_size k_circle_vertices{16*4};

  template<rx_size E>
  void generate_polygon(const math::vec2f (&coordinates)[E],
    rx_f32 _thickness, const math::vec4f& _color, queue::command::type _from_type);

  void generate_rectangle(const math::vec2f& _position, const math::vec2f& _size,
    rx_f32 _roundness, const math::vec4f& _color);

  void generate_line(const math::vec2f& _point_a,
    const math::vec2f& _point_b, rx_f32 _thickness, rx_f32 _roundness,
    const math::vec4f& _color);

  void generate_text(rx_s32 _size, const char* _font, rx_size _font_length,
    const char* _contents, rx_size _contents_length, rx_f32 _scale,
    const math::vec2f& _position, text_align _align, const math::vec4f& _color);

  void add_batch(rx_size _offset, queue::command::type _type,
    frontend::texture2D* _texture = nullptr);

  struct vertex {
    math::vec2f position;
    math::vec2f coordinate;
    math::vec4f color;
  };

  struct batch {
    rx_size offset;
    rx_size count;
    queue::command::type kind;
    frontend::state render_state;
    frontend::texture2D* texture;
  };

  frontend::interface* m_frontend;
  frontend::technique* m_technique;

  // loaded fonts
  map<font::key, font*> m_fonts;

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
  frontend::buffer* m_buffers[k_buffers];
  queue m_render_queue[k_buffers];
};

inline bool immediate2D::queue::box::operator!=(const box& _box) const {
  return _box.position != position || _box.size != size;
}

inline bool immediate2D::queue::rectangle::operator!=(const rectangle& _rectangle) const {
  return box::operator!=(_rectangle) || _rectangle.roundness != roundness;
}

inline bool immediate2D::queue::line::operator!=(const line& _line) const {
  return _line.points[0] != points[0] || _line.points[1] != points[1] ||
    _line.roundness != roundness || _line.thickness != thickness;
}

inline bool immediate2D::queue::text::operator!=(const text& _text) const {
  return _text.position != position || _text.size != size || _text.scale != scale
    || _text.font_index != font_index || _text.font_length != font_length
    || _text.text_index != text_index || _text.text_length != text_length;
}

inline constexpr immediate2D::queue::command::command()
  : kind{queue::command::type::k_uninitialized}
  , flags{0}
  , hash{0}
  , color{}
  , as_nat{}
{
}

inline void immediate2D::queue::record_text(const char* _font,
  const math::vec2i& _position, rx_s32 _size, rx_f32 _scale, text_align _align,
  const char* _contents, const math::vec4f& _color)
{
  record_text(_font, strlen(_font), _position, _size, _scale, _align, _contents,
    strlen(_contents), _color);
}

inline void immediate2D::queue::record_text(const string& _font,
  const math::vec2i& _position, rx_s32 _size, rx_f32 _scale, text_align _align,
  const string& _contents, const math::vec4f& _color)
{
  record_text(_font.data(), _font.size(), _position, _size, _scale, _align,
    _contents.data(), _contents.size(), _color);
}

inline bool immediate2D::queue::is_empty() const {
  return m_commands.is_empty();
}

inline rx_size immediate2D::font::key::hash() const {
  return hash_combine(name.hash(), rx::hash<rx_s32>{}(size));
}

inline immediate2D::queue& immediate2D::frame_queue() {
  return m_queue;
}

inline bool immediate2D::font::key::operator==(const key& _key) const {
  return name == _key.name && size == _key.size;
}

inline immediate2D::font::glyph immediate2D::font::glyph_for_code(rx_u32 _code) const {
  return m_glyphs[static_cast<rx_size>(_code)];
}

inline rx_s32 immediate2D::font::size() const {
  return m_size;
}

inline frontend::texture2D* immediate2D::font::texture() const {
  return m_texture;
}

} // namespace rx::render

#endif // RX_RENDER_IMMEDIATE_H