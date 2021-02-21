#ifndef RX_RENDER_LENS_DISTORTION_PASS_H
#define RX_RENDER_LENS_DISTORTION_PASS_H
#include "rx/math/camera.h"
#include "rx/math/vec2.h"

#include "rx/core/optional.h"

#include "rx/core/utility/exchange.h"

namespace Rx::Render {

namespace Frontend {

struct Context;
struct Technique;
struct Texture2D;
struct Target;

} // namespace Frontend

struct LensDistortionPass {
  RX_MARK_NO_COPY(LensDistortionPass);

  constexpr LensDistortionPass();
  LensDistortionPass(LensDistortionPass&& pass_);
  ~LensDistortionPass();

  LensDistortionPass& operator=(LensDistortionPass&& pass_);

  static Optional<LensDistortionPass> create(
    Frontend::Context* _frontend, const Math::Vec2z& _resolution);

  void render(Frontend::Texture2D* _source);

  bool resize(const Math::Vec2z& _resolution);

  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

  Float32 scale = 0.9f;
  Float32 dispersion = 0.01f;
  Float32 distortion = 0.1f;

private:
  constexpr LensDistortionPass(
    Frontend::Context* _frontend, Frontend::Technique* _technique,
    Frontend::Texture2D* _texture, Frontend::Target* _target);

  void release();

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;
  Frontend::Texture2D* m_texture;
  Frontend::Target* m_target;
};

inline constexpr LensDistortionPass::LensDistortionPass()
  : LensDistortionPass{nullptr, nullptr, nullptr, nullptr}
{
}

inline constexpr LensDistortionPass::LensDistortionPass(
  Frontend::Context* _frontend, Frontend::Technique* _technique,
  Frontend::Texture2D* _texture, Frontend::Target* _target)
  : m_frontend{_frontend}
  , m_technique{_technique}
  , m_texture{_texture}
  , m_target{_target}
{
}

inline LensDistortionPass::LensDistortionPass(LensDistortionPass&& pass_)
  : m_frontend{Utility::exchange(pass_.m_frontend, nullptr)}
  , m_technique{Utility::exchange(pass_.m_technique, nullptr)}
  , m_texture{Utility::exchange(pass_.m_texture, nullptr)}
  , m_target{Utility::exchange(pass_.m_target, nullptr)}
{
}

inline LensDistortionPass::~LensDistortionPass() {
  release();
}

inline LensDistortionPass& LensDistortionPass::operator=(LensDistortionPass&& pass_) {
  if (this != &pass_) {
    release();
    m_frontend = Utility::exchange(pass_.m_frontend, nullptr);
    m_technique = Utility::exchange(pass_.m_technique, nullptr);
    m_texture = Utility::exchange(pass_.m_texture, nullptr);
    m_target = Utility::exchange(pass_.m_target, nullptr);
  }
  return *this;
}

inline bool LensDistortionPass::resize(const Math::Vec2z& _resolution) {
  if (auto pass = create(m_frontend, _resolution)) {
    *this = Utility::move(*pass);
    return true;
  }
  return false;
}

inline Frontend::Target* LensDistortionPass::target() const {
  return m_target;
}

inline Frontend::Texture2D* LensDistortionPass::texture() const {
  return m_texture;
}

} // namespace Rx::Render

#endif // RX_RENDER_LENS_DISTORTION_PASS_H
