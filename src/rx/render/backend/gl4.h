#ifndef RX_RENDER_BACKEND_GL4_H
#define RX_RENDER_BACKEND_GL4_H
#include "rx/render/backend/context.h"

namespace Rx::Render::Backend {

struct GL4
  : Context
{
  GL4(Memory::Allocator& _allocator, void* _data);
  ~GL4();

  AllocationInfo query_allocation_info() const;
  DeviceInfo query_device_info() const;

  bool init();
  void process(const Vector<Byte*>& _commands);
  void process(Byte* _command);
  void swap();

private:
  Memory::Allocator& m_allocator;
  void* m_data;
  void* m_impl;
};

} // namespace Rx::Render::Backend

#endif // RX_RENDER_BACKEND_GL4_H
