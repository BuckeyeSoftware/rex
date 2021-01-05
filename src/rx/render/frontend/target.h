#ifndef RX_RENDER_FRONTEND_TARGET_H
#define RX_RENDER_FRONTEND_TARGET_H
#include "rx/core/vector.h"

#include "rx/render/frontend/texture.h"
#include "rx/render/frontend/resource.h"

namespace Rx::Render::Frontend {

struct Context;
struct Buffers;

struct Target
  : Resource
{
  RX_MARK_NO_COPY(Target);
  RX_MARK_NO_MOVE(Target);

  Target(Context* _frontend);

  struct Attachment {
    enum class Type : Uint8 {
      TEXTURE2D,
      TEXTURECM
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

    bool texture_is(const Texture* _texture) const;
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

  bool has_feedback(const Texture* _texture, const Buffers& _draw_buffers) const;

  bool owns_depth() const;
  bool owns_stencil() const;
  bool owns_depth_stencil() const;

  const Math::Vec2z& dimensions() const;

  void validate() const;

private:
  bool has_texture(const Texture* _texture) const;
  bool has_cubemap_face(const TextureCM* _texture, TextureCM::Face _face) const;

  void destroy();

  friend struct Context;

  void update_resource_usage();

  enum /* m_flags */ {
    HAS_DEPTH    = 1 << 0,
    HAS_STENCIL  = 1 << 1,
    OWNS_STENCIL = 1 << 2,
    OWNS_DEPTH   = 1 << 3,
    DIMENSIONS   = 1 << 4,
    SWAPCHAIN    = 1 << 5
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

// Target::Attachment
inline bool Target::Attachment::texture_is(const Texture* _texture) const {
  switch (kind) {
  case Type::TEXTURE2D:
    return as_texture2D.texture == _texture;
  case Type::TEXTURECM:
    return as_textureCM.texture == _texture;
  }
  RX_HINT_UNREACHABLE();
}

// Target
inline void Target::attach_texture(TextureCM* _texture, Size _level) {
  attach_texture(_texture, TextureCM::Face::RIGHT, _level);  // +x
  attach_texture(_texture, TextureCM::Face::LEFT, _level);   // -x
  attach_texture(_texture, TextureCM::Face::TOP, _level);    // +y
  attach_texture(_texture, TextureCM::Face::BOTTOM, _level); // -y
  attach_texture(_texture, TextureCM::Face::FRONT, _level);  // +z
  attach_texture(_texture, TextureCM::Face::BACK, _level);   // -z
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
  return m_flags & SWAPCHAIN;
}

inline bool Target::has_depth() const {
  return m_flags & HAS_DEPTH;
}

inline bool Target::has_stencil() const {
  return m_flags & HAS_STENCIL;
}

inline bool Target::has_depth_stencil() const {
  return has_depth() && has_stencil();
}

inline bool Target::owns_depth() const {
  return m_flags & OWNS_DEPTH;
}

inline bool Target::owns_stencil() const {
  return m_flags & OWNS_STENCIL;
}

inline bool Target::owns_depth_stencil() const {
  return owns_depth() && owns_stencil();
}

inline const Math::Vec2z& Target::dimensions() const {
  return m_dimensions;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_TARGET_H
