#ifndef RX_RENDER_INDIRECT_LIGHTING_PASS_H
#define RX_RENDER_INDIRECT_LIGHTING_PASS_H
#include "rx/math/vec2.h"
#include "rx/math/camera.h"

namespace rx::render {

namespace frontend {
  struct target;
  struct texture2D;
  struct technique;
  struct context;
} // namespace frontend

struct gbuffer;
struct ibl;

struct indirect_lighting_pass {
  indirect_lighting_pass(frontend::context* _frontend, const gbuffer* _gbuffer, const ibl* _ibl);
  ~indirect_lighting_pass();

  void render(const math::camera& _camera);

  void create(const math::vec2z& _resolution);
  void resize(const math::vec2z& _resolution);

  frontend::texture2D* texture() const;
  frontend::target* target() const;

private:
  void create();
  void destroy();

  frontend::context* m_frontend;
  frontend::technique* m_technique;
  frontend::texture2D* m_texture;
  frontend::target* m_target;

  const gbuffer* m_gbuffer;
  const ibl* m_ibl;
};

inline frontend::texture2D* indirect_lighting_pass::texture() const {
  return m_texture;
}

inline frontend::target* indirect_lighting_pass::target() const {
  return m_target;
}

} // namespace rx::render

#endif // RX_RENDER_INDIRECT_LIGHTING_PASS_H
