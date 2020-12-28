#ifndef RX_RENDER_BACKEND_ES3_H
#define RX_RENDER_BACKEND_ES3_H
#include "rx/render/backend/context.h"

namespace Rx::Render::Backend {

struct ES3
  : Context
{
  ES3(Memory::Allocator& _allocator, void* _data);
  ~ES3();

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

#endif // RX_RENDER_BACKEND_ES3_H
