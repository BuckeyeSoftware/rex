#ifndef RX_RENDER_COMMAND_H
#define RX_RENDER_COMMAND_H

#include <rx/render/state.h>

#include <rx/core/memory/stack_allocator.h>
#include <rx/core/utility/construct.h>

namespace rx::render {

struct target;
struct buffer;
struct texture1D;
struct texture2D;
struct texture3D;
struct textureCM;

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
  target* destination_target;
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
  rx_u64 binds; // bitstring of '123c', '0' indicates end of string
  const rx_byte* textures() const;
};

inline const rx_byte* draw_command::textures() const {
  return reinterpret_cast<const rx_byte*>(this) + sizeof *this;
}

} // namespace rx::render

#endif // RX_RENDER_COMMAND_H
