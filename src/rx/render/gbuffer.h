#ifndef RX_RENDER_GBUFFER_H
#define RX_RENDER_GBUFFER_H
#include "rx/math/vec2.h"

namespace Rx::Render {

namespace Frontend {
  struct Context;
  struct Target;
  struct Texture2D;
}

struct GBuffer {
  GBuffer(Frontend::Context* _frontend);
  ~GBuffer();

  void create(const Math::Vec2z& _resolution);
  void resize(const Math::Vec2z& _resolution);

  Frontend::Texture2D* albedo() const;
  Frontend::Texture2D* normal() const;
  Frontend::Texture2D* emission() const;
  Frontend::Texture2D* depth_stencil() const;

  Frontend::Target* target() const;

private:
  void destroy();

  Frontend::Context* m_frontend;
  Frontend::Target* m_target;
  Frontend::Texture2D* m_albedo_texture;
  Frontend::Texture2D* m_normal_texture;
  Frontend::Texture2D* m_emission_texture;
};

inline Frontend::Texture2D* GBuffer::albedo() const {
  return m_albedo_texture;
}

inline Frontend::Texture2D* GBuffer::normal() const {
  return m_normal_texture;
}

inline Frontend::Texture2D* GBuffer::emission() const {
  return m_emission_texture;
}

inline Frontend::Target* GBuffer::target() const {
  return m_target;
}

} // namespace Rx::Render

#endif // RX_RENDER_GBUFFER
