#ifndef RX_RENDER_FRONTEND_H
#define RX_RENDER_FRONTEND_H

#include <rx/render/command.h>

#include <rx/core/array.h>
#include <rx/core/memory/pool_allocator.h>
#include <rx/core/concurrency/mutex.h>

namespace rx::render {

struct buffer;
struct target;
struct program;
struct texture1D;
struct texture2D;
struct texture3D;
struct textureCM;

struct backend;

struct frontend {
  struct allocation_info {
    rx_size buffer_size;
    rx_size target_size;
    rx_size program_size;
    rx_size texture1D_size;
    rx_size texture2D_size;
    rx_size texture3D_size;
    rx_size textureCM_size;
  };

  frontend(memory::allocator* _allocator, backend* _backend, const allocation_info& _allocation_info);
  ~frontend();

  buffer* create_buffer(const command_header::info& _info);
  target* create_target(const command_header::info& _info);
  program* create_program(const command_header::info& _info);

  texture1D* create_texture1D(const command_header::info& _info);
  texture2D* create_texture2D(const command_header::info& _info);
  texture3D* create_texture3D(const command_header::info& _info);
  textureCM* create_textureCM(const command_header::info& _info);

  void initialize_buffer(const command_header::info& _info, buffer* _buffer);
  void initialize_target(const command_header::info& _info, target* _target);
  void initialize_program(const command_header::info& _info, program* _program);
  void initialize_texture(const command_header::info& _info, texture1D* _texture);
  void initialize_texture(const command_header::info& _info, texture2D* _texture);
  void initialize_texture(const command_header::info& _info, texture3D* _texture);
  void initialize_texture(const command_header::info& _info, textureCM* _texture);

  void destroy_buffer(const command_header::info& _info, buffer* _buffer);
  void destroy_target(const command_header::info& _info, target* _target);
  void destroy_program(const command_header::info& _info, program* _program);
  void destroy_texture(const command_header::info& _info, texture1D* _texture);
  void destroy_texture(const command_header::info& _info, texture2D* _texture);
  void destroy_texture(const command_header::info& _info, texture3D* _texture);
  void destroy_texture(const command_header::info& _info, textureCM* _texture);

  void draw_elements(
    const command_header::info& _info,
    const state& _state,
    target* _target,
    buffer* _buffer,
    program* _program,
    rx_size _count,
    rx_size _offset,
    primitive_type _primitive_type,
    const char* _textures,
    ...);

  // _clear_mask can be one of
  //  RX_RENDER_CLEAR_DEPTH,
  //  RX_RENDER_CLEAR_STENCIL,
  //  RX_RENDER_CLEAR_COLOR(index)
  //  RX_RENDER_CLEAR_DEPTH | RX_RENDER_CLEAR_STENCIL
  //  any other combinaiton of flags is undefined
  //
  // _clear_color stores the color for the clear operation, for
  //  RX_RENDER_CLEAR_DEPTH, _clear_color.r stores the depth clear value
  //  RX_RENDER_CLEAR_STENCIL, _clear_color.r stores the stencil clear value
  //  RX_RENDER_CLEAR_COLOR, _clear_color stores the color clear value
  //  RX_RENDER_CLEAR_DEPTH | RX_RENDER_CLEAR_STENCIL, _clear_color.r stores the
  //  depth clear, _clear_color.g stores the stencil clear
  //
  // NOTE: in the combined depth stencil clear, the order of the bitflags does
  // not matter but the order of the values in _clear_color does, depth is alway
  // in R and stencil always in G.
  void clear(
    const command_header::info& _info,
    target* _target,
    rx_u32 _clear_mask,
    const math::vec4f& _clear_color
  );

  target* backbuffer() const;

  bool process();
  void swap();

  memory::allocator* allocator() const;

private:
  friend struct target;

  // needed by target to release depth/stencil textures without holding m_mutex
  void destroy_texture_unlocked(const command_header::info& _info, texture2D* _texture);

  concurrency::mutex m_mutex;

  memory::allocator* m_allocator;          // protected by |m_mutex|

  memory::pool_allocator m_buffer_pool;    // protected by |m_mutex|
  memory::pool_allocator m_target_pool;    // protected by |m_mutex|
  memory::pool_allocator m_program_pool;   // protected by |m_mutex|
  memory::pool_allocator m_texture1D_pool; // protected by |m_mutex|
  memory::pool_allocator m_texture2D_pool; // protected by |m_mutex|
  memory::pool_allocator m_texture3D_pool; // protected by |m_mutex|
  memory::pool_allocator m_textureCM_pool; // protected by |m_mutex|

  array<buffer*> m_destroy_buffers;        // protected by |m_mutex|
  array<target*> m_destroy_targets;        // protected by |m_mutex|
  array<program*> m_destroy_programs;      // protected by |m_mutex|
  array<texture1D*> m_destroy_textures1D;  // protected by |m_mutex|
  array<texture2D*> m_destroy_textures2D;  // protected by |m_mutex|
  array<texture3D*> m_destroy_textures3D;  // protected by |m_mutex|
  array<textureCM*> m_destroy_texturesCM;  // protected by |m_mutex|

  array<rx_byte*> m_commands;              // protected by |m_mutex|
  command_buffer m_command_buffer;         // protected by |m_mutex|

  backend* m_backend;                      // protected by |m_mutex|

  target* m_bacbuffer;
};

inline memory::allocator* frontend::allocator() const {
  return m_allocator;
}

} // namespace rx::render

#endif // RX_RENDER_FRONTEND_H
