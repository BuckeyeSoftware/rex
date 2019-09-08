#ifndef RX_RENDER_BACKEND_GL4_H
#define RX_RENDER_BACKEND_GL4_H

#include "rx/render/backend/interface.h"

namespace rx::memory {
  struct allocator;
}

namespace rx::render::backend {

struct gl4
  : interface
{
  allocation_info query_allocation_info() const;
  device_info query_device_info() const;
  gl4(memory::allocator* _allocator, void* _data);
  ~gl4();

  void process(rx_byte* _command);
  void swap();

private:
  memory::allocator* m_allocator;
  void* m_data;
  void* m_impl;
};

} // namespace rx::render::backend

#endif // RX_RENDER_BACKEND_GL4_H
