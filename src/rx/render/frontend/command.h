#ifndef RX_RENDER_FRONTEND_COMMAND_H
#define RX_RENDER_FRONTEND_COMMAND_H
#include "rx/render/frontend/state.h"

#include "rx/core/memory/bump_point_allocator.h"

#include "rx/math/vec4.h"

namespace rx::render::frontend {

struct target;
struct buffer;
struct program;
struct texture1D;
struct texture2D;
struct texture3D;
struct textureCM;

#define RX_RENDER_CLEAR_DEPTH (1 << 0)
#define RX_RENDER_CLEAR_STENCIL (1 << 1)
#define RX_RENDER_CLEAR_COLOR(INDEX) (1 << (2 + (INDEX)))

enum class primitive_type : rx_u8 {
  k_triangles,
  k_triangle_strip,
  k_points,
  k_lines
};

enum class command_type : rx_u8 {
  k_resource_allocate,
  k_resource_construct,
  k_resource_update,
  k_resource_destroy,
  k_clear,
  k_draw,
  k_blit,
  k_profile
};

struct alignas(16) command_header {
  struct info {
    constexpr info(const char* _file, const char* _description, int _line)
      : file{_file}
      , description{_description}
      , line{_line}
    {
    }
    const char* file;
    const char* description;
    int line;
  };

  command_type type;
  info tag;
};

#define RX_RENDER_TAG(description) \
  ::rx::render::frontend::command_header::info{__FILE__, (description), __LINE__}

struct command_buffer {
  command_buffer(memory::allocator* _allocator, rx_size _size);
  ~command_buffer();

  rx_byte* allocate(rx_size _size, command_type _command,
    const command_header::info& _info);
  void reset();

  rx_size used() const;
  rx_size size() const;

private:
  memory::allocator* m_base_allocator;
  rx_byte* m_base_memory;
  memory::bump_point_allocator m_allocator;
};

inline rx_size command_buffer::used() const {
  return m_allocator.used();
}

inline rx_size command_buffer::size() const {
  return m_allocator.size();
}

struct buffers {
  static constexpr rx_size k_max_buffers{8};
  rx_u8 elements[k_max_buffers];
  rx_u8 index;
  void add(rx_u8 _buffer);
  bool operator==(const buffers& _buffers) const;
  bool operator!=(const buffers& _buffers) const;
};

struct draw_command : state {
  static constexpr const rx_size k_max_textures{16};

  target* render_target;
  buffer* render_buffer;
  program* render_program;
  rx_size count;
  rx_size offset;
  buffers draw_buffers;
  char texture_types[k_max_textures + 1];
  void* texture_binds[k_max_textures + 1];
  primitive_type type;
  rx_u64 dirty_uniforms_bitset;

  const rx_byte* uniforms() const;
  rx_byte* uniforms();
};

inline void buffers::add(rx_u8 _buffer) {
  RX_ASSERT(index < sizeof elements / sizeof *elements,
    "too many draw buffers (%d >= %d)", static_cast<int>(index),
    static_cast<int>(sizeof elements / sizeof *elements));

  elements[index++] = _buffer;
}

inline bool buffers::operator==(const buffers& _buffers) const {
  // Comparing buffers is approximate and not exact. When we share
  // a common initial sequence of elements we compare equal provided
  // the sequence length isn't larger than ours.
  if (_buffers.index > index) {
    return false;
  }

  for (rx_u8 i{0}; i < index; i++) {
    if (_buffers.elements[i] != elements[i]) {
      return false;
    }
  }

  return true;
}

inline bool buffers::operator!=(const buffers& _buffers) const {
  return !operator==(_buffers);
}

inline const rx_byte* draw_command::uniforms() const {
  // NOTE: standard permits aliasing with char (rx_byte)
  return reinterpret_cast<const rx_byte*>(this) + sizeof *this;
}

inline rx_byte* draw_command::uniforms() {
  // NOTE: standard permits aliasing with char (rx_byte)
  return reinterpret_cast<rx_byte*>(this) + sizeof *this;
}

struct clear_command : state {
  target* render_target;
  buffers draw_buffers;
  bool clear_depth;
  bool clear_stencil;
  rx_u32 clear_colors;
  rx_u8 stencil_value;
  rx_f32 depth_value;
  math::vec4f color_values[buffers::k_max_buffers];
};

struct blit_command : state {
  target* src_target;
  rx_size src_attachment;
  target* dst_target;
  rx_size dst_attachment;
};

struct profile_command {
  const char* tag;
};

struct resource_command {
  enum class type : rx_u8 {
    k_buffer,
    k_target,
    k_program,
    k_texture1D,
    k_texture2D,
    k_texture3D,
    k_textureCM
  };

  type kind;

  union {
    target* as_target;
    buffer* as_buffer;
    program* as_program;
    texture1D* as_texture1D;
    texture2D* as_texture2D;
    texture3D* as_texture3D;
    textureCM* as_textureCM;
  };
};

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_COMMAND_H
