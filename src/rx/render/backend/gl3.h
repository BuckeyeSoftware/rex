#ifndef RX_RENDER_BACKEND_GL3_H
#define RX_RENDER_BACKEND_GL3_H

#include "rx/render/backend/interface.h"

namespace rx::memory {
  struct allocator;
}

namespace rx::render::backend {

struct gl3
  : interface
{
  allocation_info query_allocation_info() const;
  device_info query_device_info() const;
  gl3(memory::allocator* _allocator, void* _data);
  ~gl3();

  void process(rx_byte* _command);
  void swap();

private:
  memory::allocator* m_allocator;
  void* m_data;
  void* m_impl;
};

} // namespace rx::render::backend

#endif // RX_RENDER_BACKEND_GL4_H
