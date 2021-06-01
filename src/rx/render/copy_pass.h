#ifndef RX_RENDER_COPY_PASS
#define RX_RENDER_COPY_PASS
#include "rx/render/frontend/texture.h"

#include "rx/math/vec2.h"

namespace Rx::Render {

namespace Frontend {
  struct Target;
  struct Technique;
  struct Context;
} // Frontend

struct CopyPass {
  RX_MARK_NO_COPY(CopyPass);

  constexpr CopyPass();
  CopyPass(CopyPass&& copy_pass_);
  ~CopyPass();
  CopyPass& operator=(CopyPass&& copy_pass_);

  struct Options {
    Math::Vec2z                     dimensions;
    Frontend::Texture2D::DataFormat format;
  };

  // Create a CopyPass.
  static Optional<CopyPass> create(Frontend::Context* _frontend,
    const Options& _options);

  void render(Frontend::Texture2D* _source);
  bool recreate(const Options& _options);

  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

private:
  CopyPass(Frontend::Context* _frontend, Frontend::Target* _target,
    Frontend::Texture2D* _texture, Frontend::Technique* _technique);

  void release();

  Frontend::Context* m_frontend;
  Frontend::Target* m_target;
  Frontend::Texture2D* m_texture;
  Frontend::Technique* m_technique;
};

// [CopyPass]
inline constexpr CopyPass::CopyPass()
  : m_frontend{nullptr}
  , m_target{nullptr}
  , m_texture{nullptr}
  , m_technique{nullptr}
{
}

inline CopyPass::CopyPass(Frontend::Context* _frontend, Frontend::Target* _target,
  Frontend::Texture2D* _texture, Frontend::Technique* _technique)
  : m_frontend{_frontend}
  , m_target{_target}
  , m_texture{_texture}
  , m_technique{_technique}
{
}

inline CopyPass::CopyPass(CopyPass&& copy_pass_)
  : m_frontend{Utility::exchange(copy_pass_.m_frontend, nullptr)}
  , m_target{Utility::exchange(copy_pass_.m_target, nullptr)}
  , m_texture{Utility::exchange(copy_pass_.m_texture, nullptr)}
  , m_technique{Utility::exchange(copy_pass_.m_technique, nullptr)}
{
}

inline CopyPass::~CopyPass() {
  release();
}

inline CopyPass& CopyPass::operator=(CopyPass&& copy_pass_) {
  if (this != &copy_pass_) {
    release();
    m_frontend = Utility::exchange(copy_pass_.m_frontend, nullptr);
    m_target = Utility::exchange(copy_pass_.m_target, nullptr);
    m_texture = Utility::exchange(copy_pass_.m_texture, nullptr);
    m_technique = Utility::exchange(copy_pass_.m_technique, nullptr);
  }
  return *this;
}

inline Frontend::Texture2D* CopyPass::texture() const {
  return m_texture;
}

inline Frontend::Target* CopyPass::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_COPY_PASS
