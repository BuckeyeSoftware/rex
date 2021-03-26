#ifndef RX_RENDER_IMMEDIATE2D_H
#define RX_RENDER_IMMEDIATE2D_H
#include "rx/core/string.h"
#include "rx/core/array.h"
#include "rx/core/string_table.h"
#include "rx/core/optional.h"
#include "rx/core/map.h"
#include "rx/core/ptr.h"

#include "rx/math/vec2.h"
#include "rx/math/vec4.h"

#include "rx/render/frontend/state.h"

namespace Rx::Render {

namespace Frontend {
  struct Buffer;
  struct Target;
  struct Technique;
  struct Texture2D;
  struct Context;
}

struct Immediate2D {
  RX_MARK_NO_COPY(Immediate2D);

  enum class TextAlign : Uint8 {
    LEFT,
    CENTER,
    RIGHT
  };

  struct Queue {
    RX_MARK_NO_COPY(Queue);

    Queue(Memory::Allocator& _allocator);
    Queue(Queue&& queue_);

    Queue& operator=(Queue&& queue_);

    struct Box {
      Math::Vec2f position;
      Math::Vec2f size;
    };

    struct Rectangle : Box {
      Float32 roundness;
    };

    struct Triangle : Box {};
    struct Scissor : Box {};

    struct Line {
      Math::Vec2f points[2];
      Float32 roundness;
      Float32 thickness;
    };

    struct Text {
      Math::Vec2f position;
      Sint32 size;
      Float32 scale;
      Size font_index;
      Size font_length;
      Size text_index;
      Size text_length;
    };

    struct Command {
      constexpr Command();

      enum class Type : Uint8 {
        UNINITIALIZED,
        RECTANGLE,
        TRIANGLE,
        LINE,
        TEXT,
        SCISSOR
      };

      Type type;
      Uint32 flags;
      Math::Vec4f color;

      union {
        struct {} as_nat;
        Line as_line;
        Text as_text;
        Rectangle as_rectangle;
        Scissor as_scissor;
        Triangle as_triangle;
      };
    };

    bool record_scissor(const Math::Vec2f& _position, const Math::Vec2f& _size);
    bool record_rectangle(const Math::Vec2f& _position, const Math::Vec2f& _size,
                          Float32 _roundness, const Math::Vec4f& _color);
    bool record_line(const Math::Vec2f& _point_a, const Math::Vec2f& _point_b,
                     Float32 _roundness, Float32 _thickness, const Math::Vec4f& _color);
    bool record_triangle(const Math::Vec2f& _position, const Math::Vec2f& _size,
                         Uint32 _flags, const Math::Vec4f& _color);

    bool record_text(const char* _font, Size _font_length,
                     const Math::Vec2f& _position, Sint32 _size, Float32 _scale, TextAlign _align,
                     const char* _contents, Size _contents_length, const Math::Vec4f& _color);

    bool record_text(const char* _font, const Math::Vec2f& _position,
                     Sint32 _size, Float32 _scale, TextAlign _align, const char* _contents,
                     const Math::Vec4f& _color);

    bool record_text(const String& _font, const Math::Vec2f& _position,
                     Sint32 _size, Float32 _scale, TextAlign _align, const String& _contents,
                     const Math::Vec4f& _color);

    void clear();

    bool is_empty() const;

  private:
    friend struct Immediate2D;

    Vector<Command> m_commands;
    StringTable m_string_table;
    Optional<Box> m_scissor;
  };

  // The downside to having to pass allocator instances to everything as a
  // dependency cannot be understated here. This is not for the faint of heart.
  Immediate2D()
    : Immediate2D{
      nullptr,
      nullptr,
      FontMap {
        Memory::SystemAllocator::instance()
      },
      Queue {
        Memory::SystemAllocator::instance()
      },
      Vector<Batch> {
        Memory::SystemAllocator::instance()
      },
      RenderBatches {
        Memory::SystemAllocator::instance(),
        Memory::SystemAllocator::instance()
      },
      RenderQueues {
        Memory::SystemAllocator::instance(),
        Memory::SystemAllocator::instance()
      },
      Buffers {
        nullptr,
        nullptr
      }
    }
  {
  }

  Immediate2D(Immediate2D&& immediate2D_);
  ~Immediate2D();

  static Optional<Immediate2D> create(Frontend::Context* _frontend);

  Immediate2D& operator=(Immediate2D&& immediate2D_);

  void render(Frontend::Target* _target);
  Queue& frame_queue();
  Frontend::Context* frontend() const;

  Float32 measure_text_length(const String& _font, const char* _text,
    Size _text_length, Sint32 _size, Float32 _scale);
  Float32 measure_text_length(const char* _font, const char* _text,
    Size _text_length, Sint32 _size, Float32 _scale);

  struct Font {
    static inline constexpr const Size DEFAULT_RESOLUTION = 128;

    struct Quad {
      Math::Vec2f position[2];
      Math::Vec2f coordinate[2];
    };

    struct Glyph {
      Math::Vec2<Uint16> position[2];
      Math::Vec2f offset;
      Float32 x_advance;
    };

    struct Key {
      Sint32 size;
      String name;
      Size hash() const;
      bool operator==(const Key& _key) const;
    };

    Font(const Key& _key, Frontend::Context* _frontend);
    Font(Font&& font_);
    ~Font();

    Quad quad_for_glyph(Size _glyph, Float32 _scale, Math::Vec2f& position_) const;
    Glyph glyph_for_code(Uint32 _code) const;

    Sint32 size() const;
    Frontend::Texture2D* texture() const;
    Frontend::Context* frontend() const;

  private:
    Frontend::Context* m_frontend;
    Sint32 m_size;
    Size m_resolution;
    Frontend::Texture2D* m_texture;
    Vector<Glyph> m_glyphs;
  };

private:
  struct Vertex {
    Math::Vec2f position;
    Math::Vec2f coordinate;
    Math::Vec4f color;
  };

  struct Batch {
    enum Type : Uint8 {
      TEXT,
      TRIANGLES,
      LINES,
    };

    Size offset;
    Size count;
    Type type;
    Frontend::State render_state;
    Frontend::Texture2D* texture;
  };

  static constexpr const Size BUFFERS = 2;
  static constexpr const Size CIRCLE_VERTICES = 16 * 4;

  template<Size E>
  void generate_polygon(const Math::Vec2f (&coordinates)[E],
    Float32 _thickness, const Math::Vec4f& _color);
  void generate_rectangle(const Math::Vec2f& _position, const Math::Vec2f& _size,
                          Float32 _roundness, const Math::Vec4f& _color);
  void generate_line(const Math::Vec2f& _point_a,
                     const Math::Vec2f& _point_b, Float32 _thickness, Float32 _roundness,
                     const Math::Vec4f& _color);
  void generate_text(Sint32 _size, const char* _font, Size _font_length,
                     const char* _contents, Size _contents_length, Float32 _scale,
                     const Math::Vec2f& _position, TextAlign _align, const Math::Vec4f& _color);
  void generate_triangle(const Math::Vec2f& _positions,
                         const Math::Vec2f& _size, const Math::Vec4f& _color);

  template<Size E>
  void size_polygon(Size& n_vertices_, Size& n_elements_);
  void size_rectangle(Float32 _roundness, Size& n_vertices_, Size& n_elements_);
  void size_line(Float32 _roundness, Size& n_vertices_, Size& n_elements_);
  void size_text(const char* _contents, Size _contents_length,
    Size& n_vertices_, Size& n_elements_);
  void size_triangle(Size& n_vertices_, Size& n_elements_);
  bool add_batch(Size _offset, Batch::Type _type, bool _blend,
                 Frontend::Texture2D* _texture = nullptr);

  void add_element(Uint32 _element);
  void add_vertex(Vertex&& vertex_);

  Ptr<Font>& access_font(const Font::Key& _key);

  void release();

  using FontMap = Map<Font::Key, Ptr<Font>>;
  using RenderBatches = Array<Vector<Batch>[BUFFERS]>;
  using RenderQueues = Array<Queue[BUFFERS]>;
  using Buffers = Array<Frontend::Buffer*[BUFFERS]>;

  Immediate2D(Frontend::Context* _frontend, Frontend::Technique* _technique,
    FontMap&& font_map_, Queue&& queue_, Vector<Batch>&& batches_,
    RenderBatches&& render_batches_, RenderQueues&& render_queues_,
    Buffers&& buffers_);

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;

  // loaded fonts
  FontMap m_fonts;

  // current scissor rectangle
  Math::Vec2i m_scissor_position;
  Math::Vec2i m_scissor_size;

  // precomputed circle vertices
  Array<Math::Vec2f[CIRCLE_VERTICES]> m_circle_vertices;

  // generated commands, vertices, elements and batches
  Queue m_queue;

  Vertex* m_vertices;
  Uint32* m_elements;

  Vector<Batch> m_batches;

  Size m_vertex_index;
  Size m_element_index;

  // buffering of batched immediates
  Size m_rd_index;
  Size m_wr_index;

  RenderBatches m_render_batches;
  RenderQueues m_render_queues;
  Buffers m_buffers;
};

// [Immediate2D::Queue]
inline constexpr Immediate2D::Queue::Command::Command()
  : type{Queue::Command::Type::UNINITIALIZED}
  , flags{0}
  , color{}
  , as_nat{}
{
}

inline bool Immediate2D::Queue::record_text(
  const String& _font, const Math::Vec2f& _position, Sint32 _size,
  Float32 _scale, TextAlign _align, const String& _contents,
  const Math::Vec4f& _color)
{
  return record_text(_font.data(), _font.size(), _position, _size, _scale, _align,
    _contents.data(), _contents.size(), _color);
}

inline bool Immediate2D::Queue::is_empty() const {
  return m_commands.is_empty();
}

inline Size Immediate2D::Font::Key::hash() const {
  return Hash::combine(name.hash(), Hash::mix_int(size));
}

inline Immediate2D::Queue& Immediate2D::frame_queue() {
  return m_queue;
}

inline Frontend::Context* Immediate2D::frontend() const {
  return m_frontend;
}

inline Float32 Immediate2D::measure_text_length(const String& _font,
  const char* _text, Size _text_length, Sint32 _size, Float32 _scale)
{
  return measure_text_length(_font.data(), _text, _text_length, _size, _scale);
}

// [Immediate2D::Font]
inline bool Immediate2D::Font::Key::operator==(const Key& _key) const {
  return name == _key.name && size == _key.size;
}

inline Immediate2D::Font::Glyph Immediate2D::Font::glyph_for_code(Uint32 _code) const {
  return m_glyphs.in_range(_code) ? m_glyphs[static_cast<Size>(_code)] : m_glyphs[0];
}

inline Sint32 Immediate2D::Font::size() const {
  return m_size;
}

inline Frontend::Texture2D* Immediate2D::Font::texture() const {
  return m_texture;
}

inline Frontend::Context* Immediate2D::Font::frontend() const {
  return m_frontend;
}

// [Immediate2D]
inline Immediate2D::Immediate2D(Immediate2D&& immediate2D_)
  : m_frontend{Utility::exchange(immediate2D_.m_frontend, nullptr)}
  , m_technique{Utility::exchange(immediate2D_.m_technique, nullptr)}
  , m_fonts{Utility::move(immediate2D_.m_fonts)}
  , m_scissor_position{Utility::exchange(immediate2D_.m_scissor_position, Math::Vec2i{})}
  , m_scissor_size{Utility::exchange(immediate2D_.m_scissor_size, Math::Vec2i{})}
  , m_circle_vertices{Utility::move(m_circle_vertices)}
  , m_queue{Utility::move(immediate2D_.m_queue)}
  , m_vertices{Utility::exchange(immediate2D_.m_vertices, nullptr)}
  , m_elements{Utility::exchange(immediate2D_.m_elements, nullptr)}
  , m_batches{Utility::move(immediate2D_.m_batches)}
  , m_vertex_index{Utility::exchange(immediate2D_.m_vertex_index, 0)}
  , m_element_index{Utility::exchange(immediate2D_.m_element_index, 0)}
  , m_rd_index{Utility::exchange(immediate2D_.m_rd_index, 0)}
  , m_wr_index{Utility::exchange(immediate2D_.m_wr_index, 0)}
  , m_render_batches{Utility::move(immediate2D_.m_render_batches)}
  , m_render_queues{Utility::move(immediate2D_.m_render_queues)}
  , m_buffers{Utility::move(immediate2D_.m_buffers)}
{
}

inline Immediate2D::~Immediate2D() {
  release();
}

inline Immediate2D& Immediate2D::operator=(Immediate2D&& immediate2D_) {
  if (this != &immediate2D_) {
    release();
    m_frontend = Utility::exchange(immediate2D_.m_frontend, nullptr);
    m_technique = Utility::exchange(immediate2D_.m_technique, nullptr);
    m_fonts = Utility::move(immediate2D_.m_fonts);
    m_scissor_position = Utility::exchange(immediate2D_.m_scissor_position, Math::Vec2i{});
    m_scissor_size = Utility::exchange(immediate2D_.m_scissor_size, Math::Vec2i{});
    m_circle_vertices = Utility::move(m_circle_vertices);
    m_queue = Utility::move(immediate2D_.m_queue);
    m_vertices = Utility::exchange(immediate2D_.m_vertices, nullptr);
    m_elements = Utility::exchange(immediate2D_.m_elements, nullptr);
    m_batches = Utility::move(immediate2D_.m_batches);
    m_vertex_index = Utility::exchange(immediate2D_.m_vertex_index, 0);
    m_element_index = Utility::exchange(immediate2D_.m_element_index, 0);
    m_rd_index = Utility::exchange(immediate2D_.m_rd_index, 0);
    m_wr_index = Utility::exchange(immediate2D_.m_wr_index, 0);
    m_render_batches = Utility::move(immediate2D_.m_render_batches);
    m_render_queues = Utility::move(immediate2D_.m_render_queues);
    m_buffers = Utility::move(immediate2D_.m_buffers);
  }
  return *this;
}

} // namespace Rx::Render

#endif // RX_RENDER_IMMEDIATE_H
