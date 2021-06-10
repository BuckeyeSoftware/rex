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

  constexpr GBuffer();
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

  Frontend::Context* m_frontend;
  Frontend::Target* m_target;
  Frontend::Texture2D* m_albedo_texture;
  Frontend::Texture2D* m_normal_texture;
  Frontend::Texture2D* m_emission_texture;
  Frontend::Texture2D* m_velocity_texture;
};

inline constexpr GBuffer::GBuffer()
  : GBuffer{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
{
}

inline constexpr GBuffer::GBuffer(Frontend::Context* _frontend,
  Frontend::Target* _target, Frontend::Texture2D* _albedo,
  Frontend::Texture2D* _normal, Frontend::Texture2D* _emission,
  Frontend::Texture2D* _velocity)
  : m_frontend{_frontend}
  , m_target{_target}
  , m_albedo_texture{_albedo}
  , m_normal_texture{_normal}
  , m_emission_texture{_emission}
  , m_velocity_texture{_velocity}
{
}

inline GBuffer::GBuffer(GBuffer&& gbuffer_)
  : m_frontend{Utility::exchange(gbuffer_.m_frontend, nullptr)}
  , m_target{Utility::exchange(gbuffer_.m_target, nullptr)}
  , m_albedo_texture{Utility::exchange(gbuffer_.m_albedo_texture, nullptr)}
  , m_normal_texture{Utility::exchange(gbuffer_.m_normal_texture, nullptr)}
  , m_emission_texture{Utility::exchange(gbuffer_.m_emission_texture, nullptr)}
  , m_velocity_texture{Utility::exchange(gbuffer_.m_velocity_texture, nullptr)}
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
  return m_albedo_texture;
}

inline Frontend::Texture2D* GBuffer::normal() const {
  return m_normal_texture;
}

inline Frontend::Texture2D* GBuffer::emission() const {
  return m_emission_texture;
}

inline Frontend::Texture2D* GBuffer::velocity() const {
  return m_velocity_texture;
}

inline Frontend::Target* GBuffer::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_GBUFFER
