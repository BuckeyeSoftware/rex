#ifndef RX_RENDER_COPY_PASS
#define RX_RENDER_COPY_PASS
#include "rx/core/optional.h"
#include "rx/math/vec2.h"

namespace Rx::Render {

namespace Frontend {
  struct Target;
  struct Texture2D;
  struct Technique;
  struct Context;
} // Frontend

struct CopyPass {
  RX_MARK_NO_COPY(CopyPass);

  constexpr CopyPass() = default;
  CopyPass(CopyPass&& copy_pass_);
  ~CopyPass();
  CopyPass& operator=(CopyPass&& copy_pass_);

  // Create a CopyPass.
  static Optional<CopyPass> create(Frontend::Context* _frontend,
    const Math::Vec2z& _dimensions, Frontend::Texture2D* _depth_stencil = nullptr);

  void attach_depth_stencil(Frontend::Texture2D* _depth_stencil);

  bool resize(const Math::Vec2z& _dimensions);

  void render(Frontend::Texture2D* _source);

  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

private:
  void release();

  Frontend::Context* m_frontend = nullptr;
  Frontend::Target* m_target = nullptr;
  Frontend::Texture2D* m_texture = nullptr;
  Frontend::Technique* m_technique = nullptr;
  Frontend::Texture2D* m_depth_stencil = nullptr;
};

inline CopyPass::~CopyPass() {
  release();
}

inline CopyPass& CopyPass::operator=(CopyPass&& copy_pass_) {
  release();
  Utility::construct<CopyPass>(this, Utility::move(copy_pass_));
  return *this;
}

inline void CopyPass::attach_depth_stencil(Frontend::Texture2D* _depth_stencil) {
  m_depth_stencil = _depth_stencil;
}

inline bool CopyPass::resize(const Math::Vec2z& _dimensions) {
  if (auto recreate = create(m_frontend, _dimensions, m_depth_stencil)) {
    *this = Utility::move(*recreate);
    return true;
  }
  return false;
}

inline Frontend::Texture2D* CopyPass::texture() const {
  return m_texture;
}

inline Frontend::Target* CopyPass::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_COPY_PASS
