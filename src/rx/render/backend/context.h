#ifndef RX_RENDER_BACKEND_CONTEXT_H
#define RX_RENDER_BACKEND_CONTEXT_H
#include "rx/core/vector.h"
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

struct device_info {
  const char* vendor;
  const char* renderer;
  const char* version;
};

struct context
  : concepts::interface
{
  virtual allocation_info query_allocation_info() const = 0;
  virtual device_info query_device_info() const = 0;
  virtual bool init() = 0;
  virtual void process(const vector<rx_byte*>& _commands) = 0;
  virtual void swap() = 0;
};

} // namespace rx::render::backend

#endif // RX_RENDER_BACKEND_CONTEXT_H
