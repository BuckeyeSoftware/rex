#ifndef RX_RENDER_FRONTEND_RESOURCE_H
#define RX_RENDER_FRONTEND_RESOURCE_H
#include "rx/core/types.h"
#include "rx/core/concepts/no_copy.h"

namespace rx::render::frontend {

struct interface;

struct resource
  : concepts::no_copy
{
  enum class type {
    k_buffer,
    k_target,
    k_program,
    k_texture1D,
    k_texture2D,
    k_texture3D,
    k_textureCM
  };

  static constexpr rx_size count();

  resource(interface* _frontend, type _type);
  ~resource();

  void update_resource_usage(rx_size _bytes);

protected:
  interface* m_frontend;

private:
  type m_resource_type;
  rx_size m_resource_usage;
};

inline constexpr rx_size resource::count() {
  return static_cast<rx_size>(type::k_textureCM) + 1;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_RESOURCE_H
