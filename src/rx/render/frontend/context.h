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

namespace Rx::Render::Frontend {

struct Buffer;
struct Target;
struct Program;
struct Texture1D;
struct Texture2D;
struct Texture3D;
struct TextureCM;
struct Technique;
struct Module;
struct Material;

struct Context {
  Context(Memory::Allocator& _allocator, Backend::Context* _backend);
  ~Context();

  // Create rendering resources.
  Buffer* create_buffer(const CommandHeader::Info& _info);
  Target* create_target(const CommandHeader::Info& _info);
  Program* create_program(const CommandHeader::Info& _info);
  Texture1D* create_texture1D(const CommandHeader::Info& _info);
  Texture2D* create_texture2D(const CommandHeader::Info& _info);
  Texture3D* create_texture3D(const CommandHeader::Info& _info);
  TextureCM* create_textureCM(const CommandHeader::Info& _info);

  // Initialize rendering resources.
  void initialize_buffer(const CommandHeader::Info& _info, Buffer* _buffer);
  void initialize_target(const CommandHeader::Info& _info, Target* _target);
  void initialize_program(const CommandHeader::Info& _info, Program* _program);
  void initialize_texture(const CommandHeader::Info& _info, Texture1D* _texture);
  void initialize_texture(const CommandHeader::Info& _info, Texture2D* _texture);
  void initialize_texture(const CommandHeader::Info& _info, Texture3D* _texture);
  void initialize_texture(const CommandHeader::Info& _info, TextureCM* _texture);

  // Update rendering resources.
  void update_buffer(const CommandHeader::Info& _info, Buffer* _buffer);
  void update_texture(const CommandHeader::Info& _info, Texture1D* _texture);
  void update_texture(const CommandHeader::Info& _info, Texture2D* _texture);
  void update_texture(const CommandHeader::Info& _info, Texture3D* _texture);

  // Destroy rendering resources.
  void destroy_buffer(const CommandHeader::Info& _info, Buffer* _buffer);
  void destroy_target(const CommandHeader::Info& _info, Target* _target);
  void destroy_program(const CommandHeader::Info& _info, Program* _program);
  void destroy_texture(const CommandHeader::Info& _info, Texture1D* _texture);
  void destroy_texture(const CommandHeader::Info& _info, Texture2D* _texture);
  void destroy_texture(const CommandHeader::Info& _info, Texture3D* _texture);
  void destroy_texture(const CommandHeader::Info& _info, TextureCM* _texture);

  // Renders |_count| geometric primitives on |_target| with specified draw
  // buffer layout |_draw_buffers| and state |_state| from array data at
  // |_offset| in |_buffer| of Type |_primitive_type| with textures described
  // by |_draw_textures|.
  void draw(
          const CommandHeader::Info& _info,
          const State& _state,
          Target* _target,
          const Buffers& _draw_buffers,
          Buffer* _buffer,
          Program* _program,
          Size _count,
          Size _offset,
          PrimitiveType _primitive_type,
          const Textures& _draw_textures);

  // Performs a clear operation on |_target| with specified draw buffer layout
  // |_draw_buffers| and state |_state|. The clear mask specified by
  // |_clear_mask| describes the packet layout of |...|.
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
  //  buffers draw_buffers;
  //  draw_buffers.add(3);
  //  draw_buffers.add(1);
  //  draw_buffers.add(0);
  //
  //  clear(
  //    RX_RENDER_TAG("annotation"),
  //    {},
  //    target,
  //    draw_buffers,
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
          const CommandHeader::Info& _info,
          const State& _state,
          Target* _target,
          const Buffers& _draw_buffers,
          Uint32 _clear_mask,
          ...
  );

  // Performs a blit from |_src| attachment |_src_attachment| to |_dst| attachment
  // |_dst_attachment|.
  //
  // The blit considers depth, stencil and scissor state specified in |_state|.
  void blit(
          const CommandHeader::Info& _info,
          const State& _state,
          Target* _src,
          Size _src_attachment,
          Target* _dst,
          Size _dst_attachment
  );

  // Used by profile::gpu_sample to insert profile markers. The backend is
  // supposed to consume the k_profile command and when a tag is specified,
  // begin timing some commands. When a tag is not specified, i.e nullptr is
  // passed, stop the timing.
  //
  // The |_tag| passed here must be a static string literal. The lifetime
  // must exist for the lifetime of the whole program.
  void profile(const char* _tag);

  void resize(const Math::Vec2z& _resolution);

  bool process();
  bool swap();

  Buffer* cached_buffer(const String& _key);
  Target* cached_target(const String& _key);
  Texture1D* cached_texture1D(const String& _key);
  Texture2D* cached_texture2D(const String& _key);
  Texture3D* cached_texture3D(const String& _key);
  TextureCM* cached_textureCM(const String& _key);

  // Pin a given resource to the render cache with the given |_key| allowing
  // it to be reused by checking the cache with the above functions.
  void cache_buffer(Buffer* _buffer, const String& _key);
  void cache_target(Target* _target, const String& _key);
  void cache_texture(Texture1D* _texture, const String& _key);
  void cache_texture(Texture2D* _texture, const String& _key);
  void cache_texture(Texture3D* _texture, const String& _key);
  void cache_texture(TextureCM* _texture, const String& _key);

  constexpr Memory::Allocator& allocator() const;

  struct Statistics {
    Size total;
    Size used;
    Size cached;
    Size memory;
  };

  struct DeviceInfo {
    constexpr DeviceInfo(Memory::Allocator& _allocator);
    String vendor;
    String renderer;
    String version;
  };

  Statistics stats(Resource::Type _type) const;

  Size draw_calls() const;
  Size clear_calls() const;
  Size blit_calls() const;
  Size vertices() const;
  Size triangles() const;
  Size lines() const;
  Size points() const;

  Target* swapchain() const;

  Technique* find_technique_by_name(const char* _name);

  const FrameTimer& timer() const &;
  const CommandBuffer& get_command_buffer() const &;
  const DeviceInfo& get_device_info() const &;

private:
  friend struct Target;
  friend struct Resource;

  // Needed by target to release depth/stencil textures without holding
  // the non-recursive mutex |m_mutex|.
  void destroy_texture_unlocked(const CommandHeader::Info& _info,
                                Texture2D* _texture);

  // Remove a given object |_object| from the cache |_cache|.
  template<typename T>
  void remove_from_cache(Map<String, T*>& cache_, T* _object);

  mutable Concurrency::Mutex m_mutex;

  Memory::Allocator& m_allocator;              // protected by |m_mutex|
  Backend::Context* m_backend;                 // protected by |m_mutex|

  // size of resources as reported by the backend
  Backend::AllocationInfo m_allocation_info;

  StaticPool m_buffer_pool;                   // protected by |m_mutex|
  StaticPool m_target_pool;                   // protected by |m_mutex|
  StaticPool m_program_pool;                  // protected by |m_mutex|
  StaticPool m_texture1D_pool;                // protected by |m_mutex|
  StaticPool m_texture2D_pool;                // protected by |m_mutex|
  StaticPool m_texture3D_pool;                // protected by |m_mutex|
  StaticPool m_textureCM_pool;                // protected by |m_mutex|

  Vector<Buffer*> m_destroy_buffers;           // protected by |m_mutex|
  Vector<Target*> m_destroy_targets;           // protected by |m_mutex|
  Vector<Program*> m_destroy_programs;         // protected by |m_mutex|
  Vector<Texture1D*> m_destroy_textures1D;     // protected by |m_mutex|
  Vector<Texture2D*> m_destroy_textures2D;     // protected by |m_mutex|
  Vector<Texture3D*> m_destroy_textures3D;     // protected by |m_mutex|
  Vector<TextureCM*> m_destroy_texturesCM;     // protected by |m_mutex|

  Target* m_swapchain_target;                  // protected by |m_mutex|
  Texture2D* m_swapchain_texture;              // protected by |m_mutex|

  Vector<Byte*> m_commands;                 // protected by |m_mutex|
  CommandBuffer m_command_buffer;             // protected by |m_mutex|

  Map<String, Buffer*> m_cached_buffers;       // protected by |m_mutex|
  Map<String, Target*> m_cached_targets;       // protected by |m_mutex|
  Map<String, Texture1D*> m_cached_textures1D; // protected by |m_mutex|
  Map<String, Texture2D*> m_cached_textures2D; // protected by |m_mutex|
  Map<String, Texture3D*> m_cached_textures3D; // protected by |m_mutex|
  Map<String, TextureCM*> m_cached_texturesCM; // protected by |m_mutex|

  // NOTE(dweiler): This has to come before techniques and modules. Everything
  // above must stay alive for the destruction of m_techniques and m_modules
  // to work.
  DeferredFunction<void()> m_deferred_process;

  Map<String, Technique> m_techniques;         // protected by |m_mutex|
  Map<String, Module> m_modules;               // protected by |m_mutex|

  Concurrency::Atomic<Size> m_draw_calls[2];
  Concurrency::Atomic<Size> m_clear_calls[2];
  Concurrency::Atomic<Size> m_blit_calls[2];
  Concurrency::Atomic<Size> m_vertices[2];
  Concurrency::Atomic<Size> m_triangles[2];
  Concurrency::Atomic<Size> m_lines[2];
  Concurrency::Atomic<Size> m_points[2];

  Size m_resource_usage[Resource::count()];

  DeviceInfo m_device_info;
  FrameTimer m_timer;
};

inline constexpr Context::DeviceInfo::DeviceInfo(Memory::Allocator& _allocator)
  : vendor{_allocator}
  , renderer{_allocator}
  , version{_allocator}
{
}

template<typename T>
inline void Context::remove_from_cache(Map<String, T*>& cache_, T* _object) {
  cache_.each_pair([&](const String& _key, T* _value) {
    if (_value != _object) {
      return true;
    }
    cache_.erase(_key);
    return false;
  });
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Context::allocator() const {
  return m_allocator;
}

inline Size Context::draw_calls() const {
  return m_draw_calls[1].load();
}

inline Size Context::clear_calls() const {
  return m_clear_calls[1].load();
}

inline Size Context::blit_calls() const {
  return m_blit_calls[1].load();
}

inline Size Context::vertices() const {
  return m_vertices[1].load();
}

inline Size Context::triangles() const {
  return m_triangles[1].load();
}

inline Size Context::lines() const {
  return m_lines[1].load();
}

inline Size Context::points() const {
  return m_points[1].load();
}

inline Target* Context::swapchain() const {
  return m_swapchain_target;
}

inline const FrameTimer& Context::timer() const & {
  return m_timer;
}

inline const CommandBuffer& Context::get_command_buffer() const & {
  return m_command_buffer;
}

inline const Context::DeviceInfo& Context::get_device_info() const & {
  return m_device_info;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_CONTEXT_H
