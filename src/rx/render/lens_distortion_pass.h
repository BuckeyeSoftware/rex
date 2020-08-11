#ifndef RX_RENDER_LENS_DISTORTION_PASS_H
#define RX_RENDER_LENS_DISTORTION_PASS_H
#include "rx/math/camera.h"
#include "rx/math/vec2.h"

namespace Rx::Render {

namespace Frontend {

struct Context;
struct Technique;
struct Texture2D;
struct Target;

} // namespace Frontend

struct LensDistortionPass {
  LensDistortionPass(Frontend::Context* _frontend);
  ~LensDistortionPass();

  void render(Frontend::Texture2D* _source);

  void create(const Math::Vec2z& _resolution);
  void resize(const Math::Vec2z& _resolution);

  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

  Float32 scale = 0.9f;
  Float32 dispersion = 0.01f;
  Float32 distortion = 0.1f;

private:
  void destroy();

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;
  Frontend::Texture2D* m_texture;
  Frontend::Target* m_target;
};

inline LensDistortionPass::~LensDistortionPass() {
  destroy();
}

inline void LensDistortionPass::resize(const Math::Vec2z& _resolution) {
  destroy();
  create(_resolution);
}

inline Frontend::Target* LensDistortionPass::target() const {
  return m_target;
}

inline Frontend::Texture2D* LensDistortionPass::texture() const {
  return m_texture;
}

} // namespace Rx::Render

#endif // RX_RENDER_LENS_DISTORTION_PASS_H
