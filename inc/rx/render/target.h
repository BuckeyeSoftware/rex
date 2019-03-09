#ifndef RX_RENDER_TARGET_H
#define RX_RENDER_TARGET_H

#include <rx/core/array.h>

#include <rx/render/texture.h>

namespace rx::render {

struct frontend;

struct target {
  target(frontend* _frontend);
  ~target();

  // request target have depth attachment
  void request_depth(texture::data_format _format);

  // request target have stencil attachment
  void request_stencil(texture::data_format _format);

  // request target have combined depth stencil attachment
  void request_depth_stencil(texture::data_format _format);

  // attach existing depth texture |_depth| to target
  void attach_depth(texture2D* _depth);

  // attach existing stencil texture |_stencil| to target
  void attach_stencil(texture2D* _stencil);

  // attach texture |_texture| to target
  void attach_texture(texture2D* _texture);

  texture2D* depth() const;
  texture2D* stencil() const;
  const array<texture2D*> attachments() const &;

private:
  enum /* m_owns */ {
    k_depth = 1 << 0,
    k_stencil = 1 << 1
  };

  frontend* m_frontend;

  union {
    struct {
      texture2D* m_depth_texture;
      texture2D* m_stencil_texture;
    };
    texture2D* m_depth_stencil_texture;
  };
  array<texture2D*> m_attachments;
  int m_owns;
};

inline texture2D* target::depth() const {
  return m_depth_texture;
}

inline texture2D* target::stencil() const {
  return m_stencil_texture;
}

inline const array<texture2D*> target::attachments() const & {
  return m_attachments;
}

} // namespace rx::render

#endif
