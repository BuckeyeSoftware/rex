#ifndef RX_RENDER_FRONTEND_INTERFACE_H
#define RX_RENDER_FRONTEND_INTERFACE_H
#include "rx/core/deferred_function.h"
#include "rx/core/vector.h"
#include "rx/core/string.h"
#include "rx/core/pool.h"
#include "rx/core/map.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/atomic.h"

#include "rx/render/frontend/command.h"
#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/timer.h"

#include "rx/render/backend/interface.h"

namespace rx::render::frontend {

struct buffer;
struct target;
struct program;
struct texture1D;
struct texture2D;
struct texture3D;
struct textureCM;
struct technique;
struct material;

struct interface {
  interface(memory::allocator* _allocator, backend::interface* _backend);
  ~interface();

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

  void update_buffer(const command_header::info& _info, buffer* _buffer);
  // TODO: update_texture functions, no program/target updates though, they're
  // fully immutable.

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

  bool process();
  bool swap();

  memory::allocator* allocator() const;

  struct statistics {
    rx_size total;
    rx_size used;
    rx_size cached;
    rx_size memory;
  };

  struct device_info {
    device_info(memory::allocator* _allocator);

    string vendor;
    string renderer;
    string version;
  };

  statistics stats(resource::type _type) const;
  rx_size draw_calls() const;
  rx_size clear_calls() const;
  rx_size vertices() const;
  rx_size triangles() const;
  rx_size lines() const;
  rx_size points() const;

  technique* find_technique_by_name(const char* _name);

  const frame_timer& timer() const &;

  const command_buffer& get_command_buffer() const &;

  const device_info& get_device_info() const &;

private:
  friend struct target;
  friend struct resource;

  // needed by target to release depth/stencil textures without holding m_mutex
  void destroy_texture_unlocked(const command_header::info& _info,
    texture2D* _texture);

  mutable concurrency::mutex m_mutex;

  memory::allocator* m_allocator;          // protected by |m_mutex|
  backend::interface* m_backend;           // protected by |m_mutex|

  // size of resources as reported by the backend
  backend::allocation_info m_allocation_info;

  pool m_buffer_pool;                      // protected by |m_mutex|
  pool m_target_pool;                      // protected by |m_mutex|
  pool m_program_pool;                     // protected by |m_mutex|
  pool m_texture1D_pool;                   // protected by |m_mutex|
  pool m_texture2D_pool;                   // protected by |m_mutex|
  pool m_texture3D_pool;                   // protected by |m_mutex|
  pool m_textureCM_pool;                   // protected by |m_mutex|

  vector<buffer*> m_destroy_buffers;       // protected by |m_mutex|
  vector<target*> m_destroy_targets;       // protected by |m_mutex|
  vector<program*> m_destroy_programs;     // protected by |m_mutex|
  vector<texture1D*> m_destroy_textures1D; // protected by |m_mutex|
  vector<texture2D*> m_destroy_textures2D; // protected by |m_mutex|
  vector<texture3D*> m_destroy_textures3D; // protected by |m_mutex|
  vector<textureCM*> m_destroy_texturesCM; // protected by |m_mutex|

  vector<rx_byte*> m_commands;             // protected by |m_mutex|
  command_buffer m_command_buffer;         // protected by |m_mutex|

  // NOTE(dweiler): Must be before m_techniques.
  deferred_function<void()> m_deferred_process;

  // NOTE(dweiler): Must be after m_deferred_process.
  map<string, technique> m_techniques;     // protected by |m_mutex|

  rx_size m_resource_usage[resource::count()];

  concurrency::atomic<rx_size> m_draw_calls[2];
  concurrency::atomic<rx_size> m_clear_calls[2];

  concurrency::atomic<rx_size> m_vertices[2];

  concurrency::atomic<rx_size> m_triangles[2];
  concurrency::atomic<rx_size> m_lines[2];
  concurrency::atomic<rx_size> m_points[2];

  device_info m_device_info;
  frame_timer m_timer;
};

inline interface::device_info::device_info(memory::allocator* _allocator)
  : vendor{_allocator}
  , renderer{_allocator}
  , version{_allocator}
{
}

inline memory::allocator* interface::allocator() const {
  return m_allocator;
}

inline rx_size interface::draw_calls() const {
  return m_draw_calls[1].load();
}

inline rx_size interface::clear_calls() const {
  return m_clear_calls[1].load();
}

inline rx_size interface::vertices() const {
  return m_vertices[1].load();
}

inline rx_size interface::triangles() const {
  return m_triangles[1].load();
}

inline rx_size interface::lines() const {
  return m_lines[1].load();
}

inline rx_size interface::points() const {
  return m_points[1].load();
}

inline const frame_timer& interface::timer() const & {
  return m_timer;
}

inline const command_buffer& interface::get_command_buffer() const & {
  return m_command_buffer;
}

inline const interface::device_info& interface::get_device_info() const & {
  return m_device_info;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_INTERFACE_H
