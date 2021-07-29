#ifndef RX_RENDER_GBUFFER_H
#define RX_RENDER_GBUFFER_H
#include "rx/math/vec2.h"

#include "rx/core/optional.h"

#include "rx/core/utility/exchange.h"

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Target;
  struct Texture2D;
} // namespace Frontend

struct GBuffer {
  RX_MARK_NO_COPY(GBuffer);

  constexpr GBuffer() = default;

  GBuffer(GBuffer&& gbuffer_);
  ~GBuffer();

  GBuffer& operator=(GBuffer&& gbuffer_);

  struct Options {
    Math::Vec2z dimensions;
  };

  static Optional<GBuffer> create(Frontend::Context* _frontend,
    const Options& _options);

  void clear();
  bool recreate(const Options& _options);

  Frontend::Texture2D* albedo() const;
  Frontend::Texture2D* normal() const;
  Frontend::Texture2D* emission() const;
  Frontend::Texture2D* velocity() const;

  Frontend::Texture2D* depth_stencil() const;
  Frontend::Target* target() const;

private:
  constexpr GBuffer(Frontend::Context* _frontend, Frontend::Target* _target,
    Frontend::Texture2D* _albedo, Frontend::Texture2D* _normal,
    Frontend::Texture2D* _emission, Frontend::Texture2D* _velocity);
  
  void release();

  Frontend::Context* m_frontend = nullptr;
  Frontend::Target* m_target = nullptr;
  Frontend::Texture2D* m_albedo = nullptr;
  Frontend::Texture2D* m_normal = nullptr;
  Frontend::Texture2D* m_emission = nullptr;
  Frontend::Texture2D* m_velocity = nullptr;
};

inline constexpr GBuffer::GBuffer(Frontend::Context* _frontend,
  Frontend::Target* _target, Frontend::Texture2D* _albedo,
  Frontend::Texture2D* _normal, Frontend::Texture2D* _emission,
  Frontend::Texture2D* _velocity)
  : m_frontend{_frontend}
  , m_target{_target}
  , m_albedo{_albedo}
  , m_normal{_normal}
  , m_emission{_emission}
  , m_velocity{_velocity}
{
}

inline GBuffer::GBuffer(GBuffer&& gbuffer_)
  : m_frontend{Utility::exchange(gbuffer_.m_frontend, nullptr)}
  , m_target{Utility::exchange(gbuffer_.m_target, nullptr)}
  , m_albedo{Utility::exchange(gbuffer_.m_albedo, nullptr)}
  , m_normal{Utility::exchange(gbuffer_.m_normal, nullptr)}
  , m_emission{Utility::exchange(gbuffer_.m_emission, nullptr)}
  , m_velocity{Utility::exchange(gbuffer_.m_velocity, nullptr)}
{
}

inline GBuffer::~GBuffer() {
  release();
}

inline bool GBuffer::recreate(const Options& _options) {
  if (auto gbuffer = create(m_frontend, _options)) {
    *this = Utility::move(*gbuffer);
    return true;
  }
  return false;
}

inline Frontend::Texture2D* GBuffer::albedo() const {
  return m_albedo;
}

inline Frontend::Texture2D* GBuffer::normal() const {
  return m_normal;
}

inline Frontend::Texture2D* GBuffer::emission() const {
  return m_emission;
}

inline Frontend::Texture2D* GBuffer::velocity() const {
  return m_velocity;
}

inline Frontend::Target* GBuffer::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_GBUFFER
