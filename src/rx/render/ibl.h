#ifndef RX_RENDER_IBL_H
#define RX_RENDER_IBL_H
#include "rx/math/vec2.h"
#include "rx/core/vector.h"

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
  irradiance_map(frontend::interface* _frontend, rx_size _resolution);
  ~irradiance_map();

  void render(frontend::textureCM* _environment_map);

  frontend::textureCM* cubemap() const;

private:
  frontend::interface* m_frontend;
  frontend::technique* m_technique;
  frontend::buffer* m_buffer;
  frontend::target* m_target;
  frontend::textureCM* m_texture;
};

struct prefilter_environment_map {
  prefilter_environment_map(frontend::interface* _frontend, rx_size _resolution);
  ~prefilter_environment_map();

  void render(frontend::textureCM* _environment_map);

  frontend::textureCM* cubemap() const;

private:
  frontend::interface* m_frontend;
  frontend::technique* m_technique;
  frontend::buffer* m_buffer;
  vector<frontend::target*> m_targets;
  frontend::textureCM* m_texture;
};

inline frontend::textureCM* irradiance_map::cubemap() const {
  return m_texture;
}

inline frontend::textureCM* prefilter_environment_map::cubemap() const {
  return m_texture;
}

} // namespace rx::render

#endif // RX_RENDER_IRRADENCE_MAP_H
