#ifndef RX_RENDER_FRONTEND_COMMAND_H
#define RX_RENDER_FRONTEND_COMMAND_H
#include "rx/core/source_location.h"
#include "rx/core/memory/bump_point_allocator.h"
#include "rx/core/utility/nat.h"

#include "rx/math/vec4.h"

#include "rx/render/frontend/state.h"

namespace rx::render::frontend {

struct target;
struct buffer;
struct program;
struct texture;
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
    constexpr info(const char* _description, const source_location& _source_location)
      : description{_description}
      , source_info{_source_location}
    {
    }
    const char* description;
    source_location source_info;
  };

  command_type type;
  info tag;
};

#define RX_RENDER_TAG(_description) \
  ::rx::render::frontend::command_header::info{(_description), RX_SOURCE_LOCATION}

struct command_buffer {
  command_buffer(memory::allocator& _allocator, rx_size _size);
  ~command_buffer();

  rx_byte* allocate(rx_size _size, command_type _command,
    const command_header::info& _info);
  void reset();

  rx_size used() const;
  rx_size size() const;

private:
  memory::allocator& m_base_allocator;
  rx_byte* m_base_memory;
  memory::bump_point_allocator m_allocator;
};

struct buffers {
  constexpr buffers();

  static constexpr rx_size k_max_buffers{8};

  void add(int _buffer);

  bool operator==(const buffers& _buffers) const;
  bool operator!=(const buffers& _buffers) const;

  rx_size size() const;
  bool is_empty() const;

  int operator[](rx_size _index) const;
  int last() const;

private:
  union {
    utility::nat m_nat;
    int m_elements[k_max_buffers];
  };
  rx_size m_index;
};

struct textures {
  static constexpr const rx_size k_max_textures{8};

  constexpr textures();

  template<typename T>
  int add(T* _texture);

  bool is_empty() const;
  rx_size size() const;

  void clear();

  texture* operator[](rx_size _index) const;

private:
  union {
    utility::nat m_nat;
    void* m_handles[k_max_textures];
  };
  rx_size m_index;
};

struct draw_command {
  buffers draw_buffers;
  textures draw_textures;
  state render_state;
  target* render_target;
  buffer* render_buffer;
  program* render_program;
  rx_size count;
  rx_size offset;
  primitive_type type;
  rx_u64 dirty_uniforms_bitset;

  const rx_byte* uniforms() const;
  rx_byte* uniforms();
};

struct clear_command {
  buffers draw_buffers;
  state render_state;
  target* render_target;
  bool clear_depth;
  bool clear_stencil;
  rx_u32 clear_colors;
  rx_u8 stencil_value;
  rx_f32 depth_value;
  math::vec4f color_values[buffers::k_max_buffers];
};

struct blit_command {
  state render_state;
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

struct update_command {
  enum class type : rx_u8 {
    k_buffer,
    k_texture1D,
    k_texture2D,
    k_texture3D
  };

  type kind;

  union {
    buffer* as_buffer;
    texture1D* as_texture1D;
    texture2D* as_texture2D;
    texture3D* as_texture3D;
  };

  // The number of edits to the resource in this update.
  rx_size edits;

  // The edit stream is an additional, variably-sized stream of data included as
  // a footer on this structure. It's contents encode a variable amount of edits
  // to the given resource.
  //
  // The encoding of the edit stream is a list of rx_size integers. The number
  // of integers per edit is determined by the resource type |kind|.
  //
  // Buffer edits are represented by a three-tuple of integers of the format
  // {
  //   sink:   sink to edit: 0 = vertex, 1 = element
  //   offset: byte offset
  //   size:   size in bytes
  // }
  //
  // Texture edits are represented by a variable-tuple of integers of the format
  // {
  //   level:  miplevel to edit
  //   offset: offset in pixels (1, 2, or 3 integers for 1D, 2D and 3D textures, respectively)
  //   size:   size in pixels (1, 2, or 3 integers for 1D, 2D and 3D textures, respectively)
  // }
  const rx_size* edit() const;
  rx_size* edit();
};

// command_buffer
inline rx_size command_buffer::used() const {
  return m_allocator.used();
}

inline rx_size command_buffer::size() const {
  return m_allocator.size();
}

// textures
inline constexpr textures::textures()
  : m_nat{}
  , m_index{0}
{
}

template<typename T>
inline int textures::add(T* _texture) {
  static_assert(
    traits::is_same<T, texture1D> ||
    traits::is_same<T, texture2D> ||
    traits::is_same<T, texture3D> ||
    traits::is_same<T, textureCM>,
    "|_texture| isn't a texture pointer");

  RX_ASSERT(m_index < k_max_textures, "too many draw textures");
  m_handles[m_index] = static_cast<void*>(_texture);
  return static_cast<int>(m_index++);
}

inline bool textures::is_empty() const {
  return m_index == 0;
}

inline rx_size textures::size() const {
  return m_index;
}

inline void textures::clear() {
  m_index = 0;
}

inline texture* textures::operator[](rx_size _index) const {
  RX_ASSERT(_index < k_max_textures, "out of bounds");
  return reinterpret_cast<texture*>(m_handles[_index]);
}

// buffers
inline constexpr buffers::buffers()
  : m_nat{}
  , m_index{0}
{
}

inline void buffers::add(int _buffer) {
  RX_ASSERT(m_index < k_max_buffers, "too many draw buffers");
  m_elements[m_index++] = _buffer;
}

inline bool buffers::operator==(const buffers& _buffers) const {
  // Comparing buffers is approximate and not exact. When we share
  // a common initial sequence of elements we compare equal provided
  // the sequence length isn't larger than ours.
  if (_buffers.m_index > m_index) {
    return false;
  }

  for (rx_size i{0}; i < m_index; i++) {
    if (_buffers.m_elements[i] != m_elements[i]) {
      return false;
    }
  }

  return true;
}

inline bool buffers::operator!=(const buffers& _buffers) const {
  return !operator==(_buffers);
}

inline rx_size buffers::size() const {
  return m_index;
}

inline bool buffers::is_empty() const {
  return m_index == 0;
}

inline int buffers::operator[](rx_size _index) const {
  RX_ASSERT(_index < k_max_buffers, "out of bounds");
  return m_elements[_index];
}

inline int buffers::last() const {
  return m_elements[m_index - 1];
}

// draw_command
inline const rx_byte* draw_command::uniforms() const {
  // NOTE: standard permits aliasing with char (rx_byte)
  return reinterpret_cast<const rx_byte*>(this) + sizeof *this;
}

inline rx_byte* draw_command::uniforms() {
  // NOTE: standard permits aliasing with char (rx_byte)
  return reinterpret_cast<rx_byte*>(this) + sizeof *this;
}

// update_command
inline const rx_size* update_command::edit() const {
  // NOTE: standard permits aliasing with char (rx_byte)
  const rx_byte *opaque{reinterpret_cast<const rx_byte*>(this) + sizeof *this};
  return reinterpret_cast<const rx_size*>(opaque);
}

inline rx_size* update_command::edit() {
  // NOTE: standard permits aliasing with char (rx_byte)
  rx_byte* opaque{reinterpret_cast<rx_byte*>(this) + sizeof *this};
  return reinterpret_cast<rx_size*>(opaque);
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_COMMAND_H
