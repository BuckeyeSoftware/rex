#ifndef RX_RENDER_BACKEND_NULL_H
#define RX_RENDER_BACKEND_NULL_H
#include "rx/render/backend/context.h"

namespace Rx::Render::Backend {

struct Null
  : Context
{
  Null(Memory::Allocator& _allocator, void* _data);
  ~Null();

  AllocationInfo query_allocation_info() const;
  DeviceInfo query_device_info() const;

  bool init();
  void process(const Vector<Byte*>& _commands);
  void swap();
};

} // namespace Rx::Render::Backend

#endif // RX_RENDER_BACKEND_GL3_H
