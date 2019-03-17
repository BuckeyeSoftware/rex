#ifndef RX_RENDER_RESOURCE_H
#define RX_RENDER_RESOURCE_H

#include <rx/core/types.h>

namespace rx::render {

struct frontend;

struct resource {
  enum class category {
    k_buffer,
    k_target,
    k_program,
    k_texture1D,
    k_texture2D,
    k_texture3D,
    k_textureCM
  };

  static constexpr rx_size count() {
    return static_cast<rx_size>(category::k_textureCM);
  }

  resource(frontend* _frontend, category _type);
  ~resource();

  void update_resource_usage(rx_size _bytes);

protected:
  frontend* m_frontend;

private:
  category m_resource_type;
};

} // namespace rx::render

#endif // RX_RENDER_RESOURCE_H
