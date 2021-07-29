#ifndef RX_RENDER_INDIRECT_LIGHTING_PASS_H
#define RX_RENDER_INDIRECT_LIGHTING_PASS_H
#include "rx/core/optional.h"

#include "rx/math/vec2.h"
#include "rx/math/camera.h"

#include "rx/core/utility/exchange.h"

namespace Rx::Render {

namespace Frontend {

struct Target;
struct Texture2D;
struct TextureCM;
struct Technique;
struct Context;

} // namespace Frontend

struct ImageBasedLighting;

struct IndirectLightingPass {
  RX_MARK_NO_COPY(IndirectLightingPass);

  constexpr IndirectLightingPass();
  IndirectLightingPass(IndirectLightingPass&& indirect_lighting_pass_);
  ~IndirectLightingPass();
  IndirectLightingPass& operator=(IndirectLightingPass&& indirect_lighting_pass_);

  struct Options {
    Frontend::Texture2D* stencil;
    Math::Vec2z dimensions;
  };

  static Optional<IndirectLightingPass> create(Frontend::Context* _frontend,
    const Options& _options);

  struct Input {
    Frontend::Texture2D* albedo     = nullptr;
    Frontend::Texture2D* normal     = nullptr;
    Frontend::Texture2D* emission   = nullptr;
    Frontend::Texture2D* depth      = nullptr;
    Frontend::TextureCM* irradiance = nullptr;
    Frontend::TextureCM* prefilter  = nullptr;
    Frontend::Texture2D* scale_bias = nullptr;
  };

  void render(const Math::Camera& _camera, const Input& _input);
  bool recreate(const Options& _options);

  // filter(false, false, false), wrap(CLAMP_TO_EDGE, CLAMP_TO_EDGE)
  Frontend::Texture2D* texture() const;
  Frontend::Target* target() const;

private:
  constexpr IndirectLightingPass(Frontend::Context* _frontend,
    Frontend::Target* _target, Frontend::Texture2D* _texture,
    Frontend::Technique* _technique);

  void release();

  Frontend::Context* m_frontend;
  Frontend::Target* m_target;
  Frontend::Texture2D* m_texture;
  Frontend::Technique* m_technique;
};

// [IndirectLightingPass]
inline constexpr IndirectLightingPass::IndirectLightingPass()
  : m_frontend{nullptr}
  , m_target{nullptr}
  , m_texture{nullptr}
  , m_technique{nullptr}
{
}

inline constexpr IndirectLightingPass::IndirectLightingPass(
  Frontend::Context* _frontend, Frontend::Target* _target,
  Frontend::Texture2D* _texture, Frontend::Technique* _technique)
  : m_frontend{_frontend}
  , m_target{_target}
  , m_texture{_texture}
  , m_technique{_technique}
{
}

inline IndirectLightingPass::IndirectLightingPass(IndirectLightingPass&& indirect_lighting_pass_)
  : m_frontend{Utility::exchange(indirect_lighting_pass_.m_frontend, nullptr)}
  , m_target{Utility::exchange(indirect_lighting_pass_.m_target, nullptr)}
  , m_texture{Utility::exchange(indirect_lighting_pass_.m_texture, nullptr)}
  , m_technique{Utility::exchange(indirect_lighting_pass_.m_technique, nullptr)}
{
}

inline IndirectLightingPass& IndirectLightingPass::operator=(IndirectLightingPass&& indirect_lighting_pass_) {
  if (this != &indirect_lighting_pass_) {
    release();
    m_frontend = Utility::exchange(indirect_lighting_pass_.m_frontend, nullptr);
    m_target = Utility::exchange(indirect_lighting_pass_.m_target, nullptr);
    m_texture = Utility::exchange(indirect_lighting_pass_.m_texture, nullptr);
    m_technique = Utility::exchange(indirect_lighting_pass_.m_technique, nullptr);
  }
  return *this;
}

inline IndirectLightingPass::~IndirectLightingPass() {
  release();
}

inline Frontend::Texture2D* IndirectLightingPass::texture() const {
  return m_texture;
}

inline Frontend::Target* IndirectLightingPass::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_INDIRECT_LIGHTING_PASS_H
