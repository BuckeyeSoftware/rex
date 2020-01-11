#ifndef RX_RENDER_BACKEND_NULL_H
#define RX_RENDER_BACKEND_NULL_H

#include "rx/render/backend/interface.h"

namespace rx::memory {
  struct allocator;
}

namespace rx::render::backend {

struct null
  : interface
{
  allocation_info query_allocation_info() const;
  device_info query_device_info() const;
  null(memory::allocator* _allocator, void* _data);
  ~null();

  bool init();
  void process(const vector<rx_byte*>& _commands);
  void swap();
};

} // namespace rx::render::backend

#endif // RX_RENDER_BACKEND_GL3_H
