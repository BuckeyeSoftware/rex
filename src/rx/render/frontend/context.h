#ifndef RX_RENDER_FRONTEND_CONTEXT_H
#define RX_RENDER_FRONTEND_CONTEXT_H
#include "rx/core/deferred_function.h"
#include "rx/core/vector.h"
#include "rx/core/string.h"
#include "rx/core/static_pool.h"
#include "rx/core/map.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/atomic.h"

#include "rx/render/frontend/command.h"
#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/timer.h"

#include "rx/render/backend/context.h"

namespace rx::render::frontend {

struct buffer;
struct target;
struct program;
struct texture1D;
struct texture2D;
struct texture3D;
struct textureCM;
struct technique;
struct module;
struct material;

struct context {
  context(memory::allocator* _allocator, backend::context* _backend);
  ~context();

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
  void update_texture(const command_header::info& _info, texture1D* _texture);
  void update_texture(const command_header::info& _info, texture2D* _texture);
  void update_texture(const command_header::info& _info, texture3D* _texture);

  void destroy_buffer(const command_header::info& _info, buffer* _buffer);
  void destroy_target(const command_header::info& _info, target* _target);
  void destroy_program(const command_header::info& _info, program* _program);
  void destroy_texture(const command_header::info& _info, texture1D* _texture);
  void destroy_texture(const command_header::info& _info, texture2D* _texture);
  void destroy_texture(const command_header::info& _info, texture3D* _texture);
  void destroy_texture(const command_header::info& _info, textureCM* _texture);

  void draw(
    const command_header::info& _info,
    const state& _state,
    target* _target,
    const buffers& _draw_buffers,
    buffer* _buffer,
    program* _program,
    rx_size _count,
    rx_size _offset,
    primitive_type _primitive_type,
    const textures& _draw_textures);

  // Performs a clear operation on |_target| with specified draw buffer layout
  // |_draw_buffers| and state |_state|. The clear mask specified by |_clear_mask|
  // describes the packet layout of |...|.
  //
  // The packet data described in |...| is passed, parsed and interpreted in
  // the following order.
  //  depth:   rx_f64 (truncated to rx_f32)
  //  stencil: rx_s32
  //  colors:  const rx_f32*
  //
  // When RX_RENDER_CLEAR_DEPTH is present in |_clear_mask|, the depth clear
  // value is expected as rx_f64 (truncated to rx_f32) in first position.
  //
  // When RX_RENDER_CLEAR_STENCIL is present in |_clear_mask|, the stencil
  // clear value is expected as rx_s32 in one of two positions depending on
  // if RX_RENDER_CLEAR_DEPTH is supplied. When RX_RENDER_CLEAR_DEPTH isn't
  // supplied, the stencil clear value is expected in first position, otherwise
  // it's expected in second position.
  //
  // When RX_RENDER_CLEAR_COLOR(n) for any |n| is present in |_clear_mask|, the
  // clear value is expected as a pointer to rx_f32 (rx_f32*) containing four
  // color values in normalized range in RGBA order. The |n| refers to the index
  // in the |_draw_buffers| specification to clear. The association of the clear
  // value in |...| and the |n| is done in order. When a RX_RENDER_CLEAR_COLOR
  // does not exist for a given |n|, the one proceeding it takes it's place,
  // thus gaps are skipped.

  //
  // Example:
  //  clear(
  //    RX_RENDER_TAG("annotation"),
  //    {},
  //    target,
  //    "310",
  //    RX_RENDER_CLEAR_DEPTH | RX_RENDER_CLEAR_STENCIL | RX_RENDER_CLEAR_COLOR(0) | RX_RENDER_CLEAR_COLOR(2),
  //    1.0f,
  //    0,
  //    math::vec4f(1.0f, 0.0f, 0.0f, 1.0f).data(),
  //    math::vec4f(0.0f, 1.0f, 0.0f, 1.0f).data());
  //
  //  The following clear will clear |target| with attachments 3, 1 and 0 enabled
  //  as draw buffers 0, 1, 2. Clearing depth to a value of 1.0, stencil to 0,
  //  draw buffer 0 (attachment 3) to red, and draw buffer 2 (attachment 0)
  //  to green, leaving draw buffer 1 (attachment 1) untouched.
  void clear(
    const command_header::info& _info,
    const state& _state,
    target* _target,
    const buffers& _draw_buffers,
    rx_u32 _clear_mask,
    ...
  );

  // Performs a blit from |_src| attachment |_src_attachment| to |_dst| attachment
  // |_dst_attachment|.
  //
  // The blit considers depth, stencil and scissor state specified in |_state|.
  void blit(
    const command_header::info& _info,
    const state& _state,
    target* _src,
    rx_size _src_attachment,
    target* _dst,
    rx_size _dst_attachment
  );

  // Used by profile::gpu_sample to insert profile markers. The backend is
  // supposed to consume the k_profile command and when a tag is specified,
  // begin timing some commands. When a tag is not specified, i.e nullptr is
  // passed, stop the timing.
  //
  // The |_tag| passed here must be a static string literal. The lifetime
  // must exist for the lifetime of the whole program.
  void profile(const char* _tag);

  void resize(const math::vec2z& _resolution);

  bool process();
  bool swap();

  buffer* cached_buffer(const string& _key);
  target* cached_target(const string& _key);
  texture1D* cached_texture1D(const string& _key);
  texture2D* cached_texture2D(const string& _key);
  texture3D* cached_texture3D(const string& _key);
  textureCM* cached_textureCM(const string& _key);

  void cache_buffer(buffer* _buffer, const string& _key);
  void cache_target(target* _target, const string& _key);
  void cache_texture(texture1D* _texture, const string& _key);
  void cache_texture(texture2D* _texture, const string& _key);
  void cache_texture(texture3D* _texture, const string& _key);
  void cache_texture(textureCM* _texture, const string& _key);

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
  rx_size blit_calls() const;
  rx_size vertices() const;
  rx_size triangles() const;
  rx_size lines() const;
  rx_size points() const;

  target* swapchain() const;

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

  template<typename T>
  void remove_from_cache(map<string, T*>& cache_, T* _object);

  mutable concurrency::mutex m_mutex;

  memory::allocator* m_allocator;              // protected by |m_mutex|
  backend::context* m_backend;                 // protected by |m_mutex|

  // size of resources as reported by the backend
  backend::allocation_info m_allocation_info;

  static_pool m_buffer_pool;                   // protected by |m_mutex|
  static_pool m_target_pool;                   // protected by |m_mutex|
  static_pool m_program_pool;                  // protected by |m_mutex|
  static_pool m_texture1D_pool;                // protected by |m_mutex|
  static_pool m_texture2D_pool;                // protected by |m_mutex|
  static_pool m_texture3D_pool;                // protected by |m_mutex|
  static_pool m_textureCM_pool;                // protected by |m_mutex|

  vector<buffer*> m_destroy_buffers;           // protected by |m_mutex|
  vector<target*> m_destroy_targets;           // protected by |m_mutex|
  vector<program*> m_destroy_programs;         // protected by |m_mutex|
  vector<texture1D*> m_destroy_textures1D;     // protected by |m_mutex|
  vector<texture2D*> m_destroy_textures2D;     // protected by |m_mutex|
  vector<texture3D*> m_destroy_textures3D;     // protected by |m_mutex|
  vector<textureCM*> m_destroy_texturesCM;     // protected by |m_mutex|

  target* m_swapchain_target;                  // protected by |m_mutex|
  texture2D* m_swapchain_texture;              // protected by |m_mutex|

  vector<rx_byte*> m_commands;                 // protected by |m_mutex|
  command_buffer m_command_buffer;             // protected by |m_mutex|

  map<string, buffer*> m_cached_buffers;       // protected by |m_mutex|
  map<string, target*> m_cached_targets;       // protected by |m_mutex|
  map<string, texture1D*> m_cached_textures1D; // protected by |m_mutex|
  map<string, texture2D*> m_cached_textures2D; // protected by |m_mutex|
  map<string, texture3D*> m_cached_textures3D; // protected by |m_mutex|
  map<string, textureCM*> m_cached_texturesCM; // protected by |m_mutex|

  // NOTE(dweiler): This has to come before techniques and modules. Everything
  // above must stay alive for the destruction of m_techniques and m_modules
  // to work.
  deferred_function<void()> m_deferred_process;

  map<string, technique> m_techniques;         // protected by |m_mutex|
  map<string, module> m_modules;               // protected by |m_mutex|

  concurrency::atomic<rx_size> m_draw_calls[2];
  concurrency::atomic<rx_size> m_clear_calls[2];
  concurrency::atomic<rx_size> m_blit_calls[2];
  concurrency::atomic<rx_size> m_vertices[2];
  concurrency::atomic<rx_size> m_triangles[2];
  concurrency::atomic<rx_size> m_lines[2];
  concurrency::atomic<rx_size> m_points[2];

  rx_size m_resource_usage[resource::count()];

  device_info m_device_info;
  frame_timer m_timer;
};

inline context::device_info::device_info(memory::allocator* _allocator)
  : vendor{_allocator}
  , renderer{_allocator}
  , version{_allocator}
{
}

template<typename T>
inline void context::remove_from_cache(map<string, T*>& cache_, T* _object) {
  cache_.each_pair([&](const string& _key, T* _value) {
    if (_value != _object) {
      return true;
    }
    cache_.erase(_key);
    return false;
  });
}

inline memory::allocator* context::allocator() const {
  return m_allocator;
}

inline rx_size context::draw_calls() const {
  return m_draw_calls[1].load();
}

inline rx_size context::clear_calls() const {
  return m_clear_calls[1].load();
}

inline rx_size context::blit_calls() const {
  return m_blit_calls[1].load();
}

inline rx_size context::vertices() const {
  return m_vertices[1].load();
}

inline rx_size context::triangles() const {
  return m_triangles[1].load();
}

inline rx_size context::lines() const {
  return m_lines[1].load();
}

inline rx_size context::points() const {
  return m_points[1].load();
}

inline target* context::swapchain() const {
  return m_swapchain_target;
}

inline const frame_timer& context::timer() const & {
  return m_timer;
}

inline const command_buffer& context::get_command_buffer() const & {
  return m_command_buffer;
}

inline const context::device_info& context::get_device_info() const & {
  return m_device_info;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_CONTEXT_H
