#ifndef RX_RENDER_INDIRECT_LIGHTING_PASS_H
#define RX_RENDER_INDIRECT_LIGHTING_PASS_H
#include "rx/math/vec2.h"
#include "rx/math/camera.h"

namespace Rx::Render {

namespace Frontend {
  struct Target;
  struct Texture2D;
  struct Technique;
  struct Context;
} // namespace frontend

struct gbuffer;
struct ImageBasedLighting;

struct IndirectLightingPass {
  IndirectLightingPass(Frontend::Context* _frontend, const gbuffer* _gbuffer, const ImageBasedLighting* _ibl);
  ~IndirectLightingPass();

  void render(const Math::camera& _camera);

  void create(const Math::Vec2z& _resolution);
  void resize(const Math::Vec2z& _resolution);

  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

private:
  void create();
  void destroy();

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;
  Frontend::Texture2D* m_texture;
  Frontend::Target* m_target;

  const gbuffer* m_gbuffer;
  const ImageBasedLighting* m_ibl;
};

inline Frontend::Texture2D* IndirectLightingPass::texture() const {
  return m_texture;
}

inline Frontend::Target* IndirectLightingPass::target() const {
  return m_target;
}

} // namespace rx::render

#endif // RX_RENDER_INDIRECT_LIGHTING_PASS_H
