#ifndef RX_RENDER_BACKEND_CONTEXT_H
#define RX_RENDER_BACKEND_CONTEXT_H
#include "rx/core/vector.h"

namespace Rx::Render::Backend {

// sizes of resources reported by the backend
struct AllocationInfo {
  Size buffer_size;
  Size target_size;
  Size program_size;
  Size texture1D_size;
  Size texture2D_size;
  Size texture3D_size;
  Size textureCM_size;
  Size downloader_size;
};

struct DeviceInfo {
  const char* vendor;
  const char* renderer;
  const char* version;
};

struct Context {
  RX_MARK_INTERFACE(Context);

  virtual AllocationInfo query_allocation_info() const = 0;
  virtual DeviceInfo query_device_info() const = 0;
  virtual bool init() = 0;
  virtual void process(const Vector<Byte*>& _commands) = 0;
  virtual void swap() = 0;
};

} // namespace Rx::Render::Backend

#endif // RX_RENDER_BACKEND_CONTEXT_H
