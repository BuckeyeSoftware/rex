#ifndef RX_RENDER_FRONTEND_TARGET_H
#define RX_RENDER_FRONTEND_TARGET_H
#include "rx/core/vector.h"

#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/resource.h"

namespace Rx::Render::Frontend {

struct Context;

struct Target : Resource {
  Target(Context* _frontend);

  struct Attachment {
    enum class Type {
      k_texture2D,
      k_textureCM
    };

    Type kind;
    Size level;

    union {
      struct {
        Texture2D* texture;
      } as_texture2D;

      struct {
        TextureCM* texture;
        TextureCM::Face face;
      } as_textureCM;
    };
  };

  // request target have depth attachment |_format| with size |_dimensions|
  void request_depth(Texture::DataFormat _format,
                     const Math::Vec2z& _dimensions);

  // request target have stencil attachment |_format| with size |_dimensions|
  void request_stencil(Texture::DataFormat _format,
                       const Math::Vec2z& _dimensions);

  // request target have combined depth stencil attachment with size |_dimensions|
  void request_depth_stencil(Texture::DataFormat _format,
                             const Math::Vec2z& _dimensions);

  // attach existing depth texture |_depth| to target
  void attach_depth(Texture2D* _depth);

  // attach existing stencil texture |_stencil| to target
  void attach_stencil(Texture2D* _stencil);

  // attach existing depth stencil texture |_depth_stencil| to target
  void attach_depth_stencil(Texture2D* _depth_stencil);

  // attach texture |_texture| level |_level| to target
  void attach_texture(Texture2D* _texture, Size _level);

  // attach cubemap face |_face| texture |_texture| level |_level| to target
  void attach_texture(TextureCM* _texture, TextureCM::Face _face, Size _level);

  // attach cubemap texture |_texture| level |_level| to target
  // attaches _all_ faces in -x, +x, -y, +y, -z, +z order
  void attach_texture(TextureCM* _texture, Size _level);

  Texture2D* depth() const;
  Texture2D* stencil() const;
  Texture2D* depth_stencil() const;
  const Vector<Attachment> attachments() const &;
  bool is_swapchain() const;

  bool has_depth() const;
  bool has_stencil() const;
  bool has_depth_stencil() const;

  const Math::Vec2z& dimensions() const;

  void validate() const;

private:
  void destroy();

  friend struct Context;

  void update_resource_usage();

  enum /* m_flags */ {
    k_has_depth    = 1 << 0,
    k_has_stencil  = 1 << 1,
    k_owns_stencil = 1 << 2,
    k_owns_depth   = 1 << 3,
    k_dimensions   = 1 << 4,
    k_swapchain    = 1 << 5
  };

  union {
    struct {
      Texture2D* m_depth_texture;
      Texture2D* m_stencil_texture;
    };
    Texture2D* m_depth_stencil_texture;
  };

  Vector<Attachment> m_attachments;
  Math::Vec2z m_dimensions;
  int m_flags;
};

inline void Target::attach_texture(TextureCM* _texture, Size _level) {
  attach_texture(_texture, TextureCM::Face::k_right, _level);  // +x
  attach_texture(_texture, TextureCM::Face::k_left, _level);   // -x
  attach_texture(_texture, TextureCM::Face::k_top, _level);    // +y
  attach_texture(_texture, TextureCM::Face::k_bottom, _level); // -y
  attach_texture(_texture, TextureCM::Face::k_front, _level);  // +z
  attach_texture(_texture, TextureCM::Face::k_back, _level);   // -z
}

inline Texture2D* Target::depth() const {
  return m_depth_texture;
}

inline Texture2D* Target::stencil() const {
  return m_stencil_texture;
}

inline Texture2D* Target::depth_stencil() const {
  return m_depth_stencil_texture;
}

inline const Vector<Target::Attachment> Target::attachments() const & {
  return m_attachments;
}

inline bool Target::is_swapchain() const {
  return m_flags & k_swapchain;
}

inline bool Target::has_depth() const {
  return m_flags & k_has_depth;
}

inline bool Target::has_stencil() const {
  return m_flags & k_has_stencil;
}

inline bool Target::has_depth_stencil() const {
  return has_depth() && has_stencil();
}

inline const Math::Vec2z& Target::dimensions() const {
  return m_dimensions;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_TARGET_H
