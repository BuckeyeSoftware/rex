#ifndef RX_RENDER_BACKEND_GL4_H
#define RX_RENDER_BACKEND_GL4_H

#include <rx/render/backend.h>
#include <rx/render/frontend.h>

namespace rx::render {

struct backend_gl4 : backend {
  backend_gl4(frontend::allocation_info& allocation_info_);
  ~backend_gl4();

  void process(rx_byte* _command);
  void swap();

private:
  void* m_impl;
};

} // namespace rx::render

#endif // RX_RENDER_BACKEND_GL4_H
