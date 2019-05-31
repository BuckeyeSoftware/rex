#ifndef RX_RENDER_BACKEND_GL4_H
#define RX_RENDER_BACKEND_GL4_H

#include "rx/render/backend.h"
#include "rx/render/frontend.h"

namespace rx::render {

struct backend_gl4 : backend {
  allocation_info query_allocation_info() const;
  backend_gl4(memory::allocator* _allocator, void* _data);
  ~backend_gl4();

  void process(rx_byte* _command);
  void swap();

private:
  memory::allocator* m_allocator;
  void* m_data;
  void* m_impl;
};

} // namespace rx::render

#endif // RX_RENDER_BACKEND_GL4_H
