#ifndef RX_RENDER_INDIRECT_LIGHTING_PASS_H
#define RX_RENDER_INDIRECT_LIGHTING_PASS_H
#include "rx/core/optional.h"

#include "rx/math/vec2.h"
#include "rx/math/camera.h"

namespace Rx::Render {

namespace Frontend {

struct Target;
struct Texture2D;
struct Technique;
struct Context;

} // namespace Frontend

struct GBuffer;
struct ImageBasedLighting;

struct IndirectLightingPass {
  RX_MARK_NO_COPY(IndirectLightingPass);

  constexpr IndirectLightingPass();
  IndirectLightingPass(IndirectLightingPass&& indirect_lighting_pass_);
  ~IndirectLightingPass();
  IndirectLightingPass& operator=(IndirectLightingPass&& indirect_lighting_pass_);

  static Optional<IndirectLightingPass> create(Frontend::Context* _frontend, const Math::Vec2z& _dimensions);

  void render(const Math::Camera& _camera, const GBuffer* _gbuffer, const ImageBasedLighting* _ibl);
  bool resize(const Math::Vec2z& _resolution);

  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

private:
  void release();

  Frontend::Context* m_frontend;
  Frontend::Technique* m_technique;
  Frontend::Texture2D* m_texture;
  Frontend::Target* m_target;
};

inline constexpr IndirectLightingPass::IndirectLightingPass()
  : m_frontend{nullptr}
  , m_technique{nullptr}
  , m_texture{nullptr}
  , m_target{nullptr}
{
}

inline IndirectLightingPass& IndirectLightingPass::operator=(IndirectLightingPass&& indirect_lighting_pass_) {
  release();
  Utility::construct<IndirectLightingPass>(this, Utility::move(indirect_lighting_pass_));
  return *this;
}

inline IndirectLightingPass::~IndirectLightingPass() {
  release();
}

inline bool IndirectLightingPass::resize(const Math::Vec2z& _dimensions) {
  if (auto recreate = create(m_frontend, _dimensions)) {
    *this = Utility::move(*recreate);
    return true;
  }
  return false;
}

inline Frontend::Texture2D* IndirectLightingPass::texture() const {
  return m_texture;
}

inline Frontend::Target* IndirectLightingPass::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_INDIRECT_LIGHTING_PASS_H
