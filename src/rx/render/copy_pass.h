#ifndef RX_RENDER_COPY_PASS
#define RX_RENDER_COPY_PASS
#include "rx/math/vec2.h"

namespace Rx::Render {

namespace Frontend {
  struct Target;
  struct Texture2D;
  struct Technique;
  struct Context;
} // Frontend

struct CopyPass {
  CopyPass(Frontend::Context* _frontend);
  ~CopyPass();

  void create(const Math::Vec2z& _dimensions);
  void attach_depth_stencil(Frontend::Texture2D* _depth_stencil);

  void resize(const Math::Vec2z& _dimensions);

  void render(Frontend::Texture2D* _source);

  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

private:
  void destroy();

  Frontend::Context* m_frontend;
  Frontend::Target* m_target;
  Frontend::Texture2D* m_texture;
  Frontend::Technique* m_technique;
  Frontend::Texture2D* m_depth_stencil;
};

inline void CopyPass::attach_depth_stencil(Frontend::Texture2D* _depth_stencil) {
  m_depth_stencil = _depth_stencil;
}

inline void CopyPass::resize(const Math::Vec2z& _dimensions) {
  destroy();
  create(_dimensions);
}

inline Frontend::Texture2D* CopyPass::texture() const {
  return m_texture;
}

inline Frontend::Target* CopyPass::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_COPY_PASS
