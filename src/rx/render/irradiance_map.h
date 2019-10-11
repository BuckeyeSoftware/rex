#ifndef RX_RENDER_IRRADIANCE_MAP_H
#define RX_RENDER_IRRADIANCE_MAP_H
#include "rx/math/vec2.h"

namespace rx::render {

namespace frontend {
  struct textureCM;
  struct target;
  struct buffer;
  struct interface;
  struct technique;
}

struct skybox;

struct irradiance_map {
  irradiance_map(frontend::interface* _frontend, const math::vec2z& _dimensions);
  ~irradiance_map();

  void render(const skybox& _skybox);

  frontend::textureCM* cubemap() const;

private:
  frontend::interface* m_frontend;
  frontend::technique* m_technique;
  frontend::buffer* m_buffer;
  frontend::target* m_target;
  frontend::textureCM* m_texture;
};

inline frontend::textureCM* irradiance_map::cubemap() const {
  return m_texture;
}

} // namespace rx::render

#endif // RX_RENDER_IRRADENCE_MAP_H
