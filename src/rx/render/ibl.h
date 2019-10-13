#ifndef RX_RENDER_IBL_H
#define RX_RENDER_IBL_H
#include "rx/core/array.h"

namespace rx::render {

namespace frontend {
  struct texture2D;
  struct textureCM;

  struct technique;
  struct interface;
} // namespace frontend

struct ibl {
  ibl(frontend::interface* _interface);
  ~ibl();

  void render(frontend::textureCM* _environment, rx_size _irradiance_map_size);

  frontend::textureCM* irradiance() const;
  frontend::textureCM* prefilter() const;
  frontend::texture2D* scale_bias() const;

private:
  frontend::interface* m_frontend;
  frontend::textureCM* m_irradiance_texture;
  frontend::textureCM* m_prefilter_texture;
  frontend::texture2D* m_scale_bias_texture;
};

inline frontend::textureCM* ibl::irradiance() const {
  return m_irradiance_texture;
}

inline frontend::textureCM* ibl::prefilter() const {
  return m_prefilter_texture;
}

inline frontend::texture2D* ibl::scale_bias() const {
  return m_scale_bias_texture;
}

} // namespace rx::render

#endif // RX_RENDER_IBL_H
