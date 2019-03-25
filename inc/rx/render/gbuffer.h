#ifndef RX_RENDER_GBUFFER_H
#define RX_RENDER_GBUFFER_H

#include <rx/math/vec2.h>

namespace rx::render {

struct target;
struct frontend;
struct texture2D;

struct gbuffer {
  gbuffer(frontend* _frontend);
  ~gbuffer();

  void create(const math::vec2z& _resolution);
  void resize(const math::vec2z& _resolution);

  texture2D* albedo() const;
  texture2D* normal() const;
  texture2D* emission() const;

  operator target*() const;

private:
  void destroy();

  frontend* m_frontend;
  target* m_target;
  texture2D* m_albedo_texture;
  texture2D* m_normal_texture;
  texture2D* m_emission_texture;
};

inline texture2D* gbuffer::albedo() const {
  return m_albedo_texture;
}

inline texture2D* gbuffer::normal() const {
  return m_normal_texture;
}

inline texture2D* gbuffer::emission() const {
  return m_emission_texture;
}

inline gbuffer::operator target*() const {
  return m_target;
}

} // namespace rx::render

#endif // RX_RENDER_GBUFFER