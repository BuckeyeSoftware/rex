#ifndef RX_RENDER_GBUFFER_H
#define RX_RENDER_GBUFFER_H

#include "rx/math/vec2.h"

namespace rx::render {

namespace frontend {
  struct interface;
  struct target;
  struct texture2D;
}

struct gbuffer {
  gbuffer(frontend::interface* _frontend);
  ~gbuffer();

  void create(const math::vec2z& _resolution);
  void resize(const math::vec2z& _resolution);

  frontend::texture2D* albedo() const;
  frontend::texture2D* normal() const;
  frontend::texture2D* emission() const;
  frontend::texture2D* depth_stencil() const;

  frontend::target* target() const;

private:
  void destroy();

  frontend::interface* m_frontend;
  frontend::target* m_target;
  frontend::texture2D* m_albedo_texture;
  frontend::texture2D* m_normal_texture;
  frontend::texture2D* m_emission_texture;
};

inline frontend::texture2D* gbuffer::albedo() const {
  return m_albedo_texture;
}

inline frontend::texture2D* gbuffer::normal() const {
  return m_normal_texture;
}

inline frontend::texture2D* gbuffer::emission() const {
  return m_emission_texture;
}

inline frontend::target* gbuffer::target() const {
  return m_target;
}

} // namespace rx::render

#endif // RX_RENDER_GBUFFER