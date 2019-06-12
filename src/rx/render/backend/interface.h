#ifndef RX_RENDER_BACKEND_INTERFACE_H
#define RX_RENDER_BACKEND_INTERFACE_H

#include "rx/core/types.h" // rx_byte
#include "rx/core/concepts/interface.h" // concepts::interface

namespace rx::render::backend {

// sizes of resources reported by the backend
struct allocation_info {
  rx_size buffer_size;
  rx_size target_size;
  rx_size program_size;
  rx_size texture1D_size;
  rx_size texture2D_size;
  rx_size texture3D_size;
  rx_size textureCM_size;
};

struct interface
  : concepts::interface
{
  virtual allocation_info query_allocation_info() const = 0;
  virtual void process(rx_byte* _command) = 0;
  virtual void swap() = 0;
};

} // namespace rx::render::backend

#endif // RX_RENDER_BACKEND_INTERFACE_H
