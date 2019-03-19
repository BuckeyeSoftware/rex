#ifndef RX_RENDER_BACKEND_H
#define RX_RENDER_BACKEND_H

#include <rx/core/concepts/interface.h>
#include <rx/core/types.h> // rx_byte

namespace rx::render {

struct backend
  : concepts::interface
{
  virtual void process(rx_byte* _command) = 0;
  virtual void swap(void* _context) = 0;
};

} // namespace rx::render

#endif
