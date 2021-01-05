#ifndef RX_RENDER_FRONTEND_CONTEXT_H
#define RX_RENDER_FRONTEND_CONTEXT_H
#include "rx/core/vector.h"
#include "rx/core/string.h"
#include "rx/core/static_pool.h"
#include "rx/core/map.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/atomic.h"

#include "rx/render/frontend/command.h"
#include "rx/render/frontend/resource.h"
#include "rx/render/frontend/arena.h"
#include "rx/render/frontend/timer.h"

#include "rx/render/backend/context.h"

namespace Rx::Render::Frontend {

// Lower-level resource types.
struct Buffer;
struct Target;
struct Program;
struct Texture1D;
struct Texture2D;
struct Texture3D;
struct TextureCM;
struct Downloader;

// Higher-level resource types.
struct Technique;
struct Module;

struct Context {
  Context(Memory::Allocator& _allocator, Backend::Context* _backend, const Math::Vec2z& _dimensions, bool _hdr);
  ~Context();

  // Create rendering resources.
  Buffer* create_buffer(const CommandHeader::Info& _info);
  Target* create_target(const CommandHeader::Info& _info);
  Program* create_program(const CommandHeader::Info& _info);
  Texture1D* create_texture1D(const CommandHeader::Info& _info);
  Texture2D* create_texture2D(const CommandHeader::Info& _info);
  Texture3D* create_texture3D(const CommandHeader::Info& _info);
  TextureCM* create_textureCM(const CommandHeader::Info& _info);
  Downloader* create_downloader(const CommandHeader::Info& _info);

  // Initialize rendering resources.
  void initialize_buffer(const CommandHeader::Info& _info, Buffer* _buffer);
  void initialize_target(const CommandHeader::Info& _info, Target* _target);
  void initialize_program(const CommandHeader::Info& _info, Program* _program);
  void initialize_texture(const CommandHeader::Info& _info, Texture1D* _texture);
  void initialize_texture(const CommandHeader::Info& _info, Texture2D* _texture);
  void initialize_texture(const CommandHeader::Info& _info, Texture3D* _texture);
  void initialize_texture(const CommandHeader::Info& _info, TextureCM* _texture);
  void initialize_downloader(const CommandHeader::Info& _info, Downloader* _downloader);

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
  void destroy_downloader(const CommandHeader::Info& _info, Downloader* _downloader);

  // Renders |_instances| instances of |_count| geometric primitive of
  // |_primitive_type| to |_target| with draw buffer layout |_draw_buffers|
  // and render |_state| from array data at |_offset| in |_buffer| with textures
  // |_draw_textures|.
  void draw(
    const CommandHeader::Info& _info,
    const State& _state,
    Target* _target,
    const Buffers& _draw_buffers,
    Buffer* _buffer,
    Program* _program,
    Size _count,
    Size _offset,
    Size _instances,
    Size _base_vertex,
    Size _base_instance,
    PrimitiveType _primitive_type,
    const Textures& _draw_textures);

  // Performs a clear operation on |_target| with specified draw buffer layout
  // |_draw_buffers| and state |_state|. The clear mask specified by
  // |_clear_mask| describes the packet layout of |...|.
  //
  // The packet data described in |...| is passed, parsed and interpreted in
  // the following order.
  //  depth:   Float64 (truncated to Float32)
  //  stencil: Sint32
  //  colors:  const Float32*
  //
  // When RX_RENDER_CLEAR_DEPTH is present in |_clear_mask|, the depth clear
  // value is expected as Float64 (truncated to Float32) in first position.
  //
  // When RX_RENDER_CLEAR_STENCIL is present in |_clear_mask|, the stencil
  // clear value is expected as Sint32 in one of two positions depending on
  // if RX_RENDER_CLEAR_DEPTH is supplied. When RX_RENDER_CLEAR_DEPTH isn't
  // supplied, the stencil clear value is expected in first position, otherwise
  // it's expected in second position.
  //
  // When RX_RENDER_CLEAR_COLOR(n) for any |n| is present in |_clear_mask|, the
  // clear value is expected as a pointer to Float32 (Float32*) containing four
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
    ...);

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
    Size _dst_attachment);

  // Performs an asyncrounous download from |_src| attachment |_src_attachment|
  // at |_offset| into the downloader object |downloader_|.
  //
  // The size and format of the downloaded data is decided upon by how the
  // downloader resource is initialized.
  //
  // Conversion of the data may incur longer download times. In general a
  // download has multi-frame latency.
  void download(
    const CommandHeader::Info& _info,
    Target* _src,
    Size _src_attachment,
    const Math::Vec2z& _offset,
    Downloader* downloader_);

  // Used by profile::gpu_sample to insert profile markers. The backend is
  // supposed to consume the |PROFILE| command and when a tag is specified,
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
  bool cache_buffer(Buffer* _buffer, const String& _key);
  bool cache_target(Target* _target, const String& _key);
  bool cache_texture(Texture1D* _texture, const String& _key);
  bool cache_texture(Texture2D* _texture, const String& _key);
  bool cache_texture(Texture3D* _texture, const String& _key);
  bool cache_texture(TextureCM* _texture, const String& _key);

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
  Size instanced_draw_calls() const;
  Size clear_calls() const;
  Size blit_calls() const;
  Size vertices() const;
  Size triangles() const;
  Size lines() const;
  Size points() const;
  Size commands() const;
  Size footprint() const;
  Uint64 frame() const;

  Target* swapchain() const;

  Technique* find_technique_by_name(const char* _name);

  Arena* arena(const Buffer::Format& _format);

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

  Memory::Allocator& m_allocator               RX_HINT_GUARDED_BY(m_mutex);
  Backend::Context* m_backend                  RX_HINT_GUARDED_BY(m_mutex);

  // size of resources as reported by the backend
  Backend::AllocationInfo m_allocation_info;

  StaticPool m_buffer_pool                     RX_HINT_GUARDED_BY(m_mutex);
  StaticPool m_target_pool                     RX_HINT_GUARDED_BY(m_mutex);
  StaticPool m_program_pool                    RX_HINT_GUARDED_BY(m_mutex);
  StaticPool m_texture1D_pool                  RX_HINT_GUARDED_BY(m_mutex);
  StaticPool m_texture2D_pool                  RX_HINT_GUARDED_BY(m_mutex);
  StaticPool m_texture3D_pool                  RX_HINT_GUARDED_BY(m_mutex);
  StaticPool m_textureCM_pool                  RX_HINT_GUARDED_BY(m_mutex);
  StaticPool m_downloader_pool                 RX_HINT_GUARDED_BY(m_mutex);

  // Resources that were destroyed are recorded into the following vectors
  // so that the destruction can be handled at the end of the frame.
  Vector<Buffer*> m_destroy_buffers            RX_HINT_GUARDED_BY(m_mutex);
  Vector<Target*> m_destroy_targets            RX_HINT_GUARDED_BY(m_mutex);
  Vector<Program*> m_destroy_programs          RX_HINT_GUARDED_BY(m_mutex);
  Vector<Texture1D*> m_destroy_textures1D      RX_HINT_GUARDED_BY(m_mutex);
  Vector<Texture2D*> m_destroy_textures2D      RX_HINT_GUARDED_BY(m_mutex);
  Vector<Texture3D*> m_destroy_textures3D      RX_HINT_GUARDED_BY(m_mutex);
  Vector<TextureCM*> m_destroy_texturesCM      RX_HINT_GUARDED_BY(m_mutex);
  Vector<Downloader*> m_destroy_downloaders    RX_HINT_GUARDED_BY(m_mutex);

  // Resources that were edited are recorded into the following vectors
  // so that the edits can be handled at the start of the frame.
  Vector<Buffer*> m_edit_buffers               RX_HINT_GUARDED_BY(m_mutex);
  Vector<Texture1D*> m_edit_textures1D         RX_HINT_GUARDED_BY(m_mutex);
  Vector<Texture2D*> m_edit_textures2D         RX_HINT_GUARDED_BY(m_mutex);
  Vector<Texture3D*> m_edit_textures3D         RX_HINT_GUARDED_BY(m_mutex);
  Vector<TextureCM*> m_edit_texturesCM         RX_HINT_GUARDED_BY(m_mutex);

  Target* m_swapchain_target                   RX_HINT_GUARDED_BY(m_mutex);
  Texture2D* m_swapchain_texture               RX_HINT_GUARDED_BY(m_mutex);

  Vector<Byte*> m_commands                     RX_HINT_GUARDED_BY(m_mutex);
  CommandBuffer m_command_buffer               RX_HINT_GUARDED_BY(m_mutex);

  Map<String, Buffer*> m_cached_buffers        RX_HINT_GUARDED_BY(m_mutex);
  Map<String, Target*> m_cached_targets        RX_HINT_GUARDED_BY(m_mutex);
  Map<String, Texture1D*> m_cached_textures1D  RX_HINT_GUARDED_BY(m_mutex);
  Map<String, Texture2D*> m_cached_textures2D  RX_HINT_GUARDED_BY(m_mutex);
  Map<String, Texture3D*> m_cached_textures3D  RX_HINT_GUARDED_BY(m_mutex);
  Map<String, TextureCM*> m_cached_texturesCM  RX_HINT_GUARDED_BY(m_mutex);

  Map<String, Technique> m_techniques          RX_HINT_GUARDED_BY(m_mutex);
  Map<String, Module> m_modules                RX_HINT_GUARDED_BY(m_mutex);

  Map<Buffer::Format, Arena> m_arenas          RX_HINT_GUARDED_BY(m_mutex);

  Concurrency::Atomic<Size> m_draw_calls[2];
  Concurrency::Atomic<Size> m_instanced_draw_calls[2];
  Concurrency::Atomic<Size> m_clear_calls[2];
  Concurrency::Atomic<Size> m_blit_calls[2];
  Concurrency::Atomic<Size> m_vertices[2];
  Concurrency::Atomic<Size> m_triangles[2];
  Concurrency::Atomic<Size> m_lines[2];
  Concurrency::Atomic<Size> m_points[2];
  Concurrency::Atomic<Size> m_commands_recorded[2];
  Concurrency::Atomic<Size> m_footprint[2];

  Uint64 m_frame;

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
void Context::remove_from_cache(Map<String, T*>& cache_, T* _object) {
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

inline Size Context::instanced_draw_calls() const {
  return m_instanced_draw_calls[1].load();
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

inline Size Context::commands() const {
  return m_commands_recorded[1].load();
}

inline Size Context::footprint() const {
  return m_footprint[1].load();
}

inline Uint64 Context::frame() const {
  return m_frame;
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

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_CONTEXT_H
