#ifndef RX_RENDER_FRONTEND_COMMAND_H
#define RX_RENDER_FRONTEND_COMMAND_H

#include "rx/render/frontend/state.h"

#include "rx/core/memory/bump_point_allocator.h"
#include "rx/core/utility/construct.h"

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
  k_triangle_strip
};

enum class command_type : rx_u8 {
  k_resource_allocate,
  k_resource_construct,
  k_resource_update,
  k_resource_destroy,
  k_clear,
  k_draw,
  k_draw_elements
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
  static constexpr const rx_size k_alignment{16};

  command_buffer(memory::allocator* _allocator, rx_size _size);
  ~command_buffer();

  rx_byte* allocate(rx_size _size, command_type _command,
    const command_header::info& _info);
  void reset();

  rx_size used() const;
  rx_size size() const;

private:
  memory::bump_point_allocator m_allocator;
};

inline rx_size command_buffer::used() const {
  return m_allocator.used();
}

inline rx_size command_buffer::size() const {
  return m_allocator.size();
}

struct clear_command {
  target* render_target;
  int clear_mask;
  math::vec4f clear_color;
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

struct draw_command : state {
  target* render_target;
  buffer* render_buffer;
  program* render_program;
  rx_size count;
  rx_size offset;
  char texture_types[8];
  void* texture_binds[8];
  primitive_type type;
  rx_u64 dirty_uniforms_bitset;
  
  const rx_byte* uniforms() const;
  rx_byte* uniforms();
};

inline const rx_byte* draw_command::uniforms() const {
  // NOTE: standard permits aliasing with char (rx_byte)
  return reinterpret_cast<const rx_byte*>(this) + sizeof *this;
}

inline rx_byte* draw_command::uniforms() {
  // NOTE: standard permits aliasing with char (rx_byte)
  return reinterpret_cast<rx_byte*>(this) + sizeof *this;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_COMMAND_H
