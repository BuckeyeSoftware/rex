#ifndef RX_RENDER_BACKEND_ES3_H
#define RX_RENDER_BACKEND_ES3_H
#include "rx/render/backend/context.h"

namespace rx::memory {
  struct allocator;
}

namespace rx::render::backend {

struct es3
  : context
{
  allocation_info query_allocation_info() const;
  device_info query_device_info() const;
  es3(memory::allocator& _allocator, void* _data);
  ~es3();

  bool init();
  void process(const vector<rx_byte*>& _commands);
  void process(rx_byte* _command);
  void swap();

private:
  memory::allocator& m_allocator;
  void* m_data;
  void* m_impl;
};

} // namespace rx::render::backend

#endif // RX_RENDER_BACKEND_ES3_H
