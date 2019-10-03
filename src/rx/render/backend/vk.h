#ifndef RX_RENDER_BACKEND_VK_H
#define RX_RENDER_BACKEND_VK_H

#include "rx/render/backend/interface.h"

namespace rx::memory {
  struct allocator;
}

namespace rx::render::backend {

struct vk
  : interface
{
  allocation_info query_allocation_info() const;
  device_info query_device_info() const;
  vk(memory::allocator* _allocator, void* _data);
  ~vk();

  void process(rx_byte* _command);
  void swap();

private:
  void* m_impl;
};

} // namespace rx::render::backend

#endif
