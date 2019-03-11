#ifndef RX_RENDER_COMMAND_H
#define RX_RENDER_COMMAND_H

#include <rx/render/state.h>

#include <rx/core/memory/stack_allocator.h>
#include <rx/core/utility/construct.h>
#include <rx/math/vec4.h>

namespace rx::render {

struct target;
struct buffer;
struct texture1D;
struct texture2D;
struct texture3D;
struct textureCM;

#define RX_RENDER_CLEAR_DEPTH (1 << 0)
#define RX_RENDER_CLEAR_STENCIL (1 << 1)
#define RX_RENDER_CLEAR_COLOR(INDEX) (1 << (2 + (INDEX)))

enum class primitive_type : rx_u8 {
  k_triangles
};

enum class command_type : rx_u8 {
  k_resource_allocate,
  k_resource_construct,
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
  ::rx::render::command_header::info{__FILE__, (description), __LINE__}

struct command_buffer {
  static constexpr const rx_size k_alignment{16};

  command_buffer(memory::allocator* _allocator, rx_size _size);
  ~command_buffer();

  rx_byte* allocate(rx_size _size, command_type _command, const command_header::info& _info);
  void reset();

private:
  memory::stack_allocator m_allocator;
};

struct clear_command {
  target* render_target;
  int clear_mask;
  math::vec4f clear_color;
};

struct resource_command {
  enum class category : rx_u8 {
    k_buffer,
    k_target,
    k_texture1D,
    k_texture2D,
    k_texture3D,
    k_textureCM
  };

  category type;

  union {
    target* as_target;
    buffer* as_buffer;
    texture1D* as_texture1D;
    texture2D* as_texture2D;
    texture3D* as_texture3D;
    textureCM* as_textureCM;
  };
};

struct draw_command : state {
  target* render_target;
  buffer* render_buffer;
  rx_size count;
  rx_size offset;
  primitive_type type;
  char texture_types[9];
  void* texture_binds[8];
};

} // namespace rx::render

#endif // RX_RENDER_COMMAND_H
