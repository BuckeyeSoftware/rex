#ifndef RX_RENDER_FRONTEND_TARGET_H
#define RX_RENDER_FRONTEND_TARGET_H
#include "rx/core/vector.h"

#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/resource.h"

namespace rx::render::frontend {

struct interface;

struct target : resource {
  target(interface* _frontend);
  ~target();

  // request the swap chain for this target
  void request_swapchain();

  // request target have depth attachment |_format| with size |_dimensions|
  void request_depth(texture::data_format _format,
    const math::vec2z& _dimensions);

  // request target have stencil attachment |_format| with size |_dimensions|
  void request_stencil(texture::data_format _format,
    const math::vec2z& _dimensions);

  // request target have combined depth stencil attachment with size |_dimensions|
  void request_depth_stencil(texture::data_format _format,
    const math::vec2z& _dimensions);

  // attach existing depth texture |_depth| to target
  void attach_depth(texture2D* _depth);

  // attach existing stencil texture |_stencil| to target
  void attach_stencil(texture2D* _stencil);

  // attach texture |_texture| to target
  void attach_texture(texture2D* _texture);

  texture2D* depth() const;
  texture2D* stencil() const;
  texture2D* depth_stencil() const;
  const vector<texture2D*> attachments() const &;
  bool is_swapchain() const;

  const math::vec2z& dimensions() const;

  void validate() const;

private:
  void update_resource_usage();

  enum /* m_flags */ {
    k_depth      = 1 << 0,
    k_stencil    = 1 << 1,
    k_dimensions = 1 << 2
  };

  union {
    struct {
      texture2D* m_depth_texture;
      texture2D* m_stencil_texture;
    };
    texture2D* m_depth_stencil_texture;
  };

  vector<texture2D*> m_attachments;
  int m_flags;
  bool m_is_swapchain;
  math::vec2z m_dimensions;
};

inline texture2D* target::depth() const {
  return m_depth_texture;
}

inline texture2D* target::stencil() const {
  return m_stencil_texture;
}

inline texture2D* target::depth_stencil() const {
  return m_depth_stencil_texture;
}

inline const vector<texture2D*> target::attachments() const & {
  return m_attachments;
}

inline bool target::is_swapchain() const {
  return m_is_swapchain;
}

inline const math::vec2z& target::dimensions() const {
  return m_dimensions;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_TARGET_H
