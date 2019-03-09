#ifndef RX_RENDER_FRONTEND_H
#define RX_RENDER_FRONTEND_H

#include <rx/render/command.h>

#include <rx/core/array.h>
#include <rx/core/memory/pool_allocator.h>
#include <rx/core/concurrency/mutex.h>

namespace rx::render {

struct buffer;
struct target;
struct texture1D;
struct texture2D;
struct texture3D;
struct textureCM;

struct backend;

struct frontend {
  frontend(memory::allocator* _allocator, backend* _backend, rx_size _command_memory);
  ~frontend();

  buffer* create_buffer(const command_header::info& _info);
  target* create_target(const command_header::info& _info);
  texture1D* create_texture1D(const command_header::info& _info);
  texture2D* create_texture2D(const command_header::info& _info);
  texture3D* create_texture3D(const command_header::info& _info);
  textureCM* create_textureCM(const command_header::info& _info);

  void initialize_buffer(const command_header::info& _info, buffer* _buffer);
  void initialize_target(const command_header::info& _info, target* _target);
  void initialize_texture(const command_header::info& _info, texture1D* _texture);
  void initialize_texture(const command_header::info& _info, texture2D* _texture);
  void initialize_texture(const command_header::info& _info, texture3D* _texture);
  void initialize_texture(const command_header::info& _info, textureCM* _texture);

  void destroy_buffer(const command_header::info& _info, buffer* _buffer);
  void destroy_target(const command_header::info& _info, target* _target);
  void destroy_texture(const command_header::info& _info, texture1D* _texture);
  void destroy_texture(const command_header::info& _info, texture2D* _texture);
  void destroy_texture(const command_header::info& _info, texture3D* _texture);
  void destroy_texture(const command_header::info& _info, textureCM* _texture);

  bool process();
  void swap();

private:
  friend struct target;

  // needed by target to release depth/stencil textures without holding m_mutex
  void destroy_texture_unlocked(const command_header::info& _info, texture2D* _texture);

  concurrency::mutex m_mutex;

  memory::pool_allocator m_buffer_pool;    // protected by |m_mutex|
  memory::pool_allocator m_target_pool;    // protected by |m_mutex|
  memory::pool_allocator m_texture1D_pool; // protected by |m_mutex|
  memory::pool_allocator m_texture2D_pool; // protected by |m_mutex|
  memory::pool_allocator m_texture3D_pool; // protected by |m_mutex|
  memory::pool_allocator m_textureCM_pool; // protected by |m_mutex|

  array<buffer*> m_destroy_buffers;        // protected by |m_mutex|
  array<target*> m_destroy_targets;        // protected by |m_mutex|
  array<texture1D*> m_destroy_textures1D;  // protected by |m_mutex|
  array<texture2D*> m_destroy_textures2D;  // protected by |m_mutex|
  array<texture3D*> m_destroy_textures3D;  // protected by |m_mutex|
  array<textureCM*> m_destroy_texturesCM;  // protected by |m_mutex|

  array<rx_byte*> m_commands;              // protected by |m_mutex|
  command_buffer m_command_buffer;         // protected by |m_mutex|

  backend* m_backend;                      // protected by |m_mutex|
};

} // namespace rx::render

#endif // RX_RENDER_FRONTEND_H
