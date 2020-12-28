#ifndef RX_RENDER_BACKEND_CONTEXT_H
#define RX_RENDER_BACKEND_CONTEXT_H
#include "rx/core/vector.h"

namespace Rx::Render::Backend {

// sizes of resources reported by the backend
struct AllocationInfo {
  Size buffer_size = 0;
  Size target_size = 0;
  Size program_size = 0;
  Size texture1D_size = 0;
  Size texture2D_size = 0;
  Size texture3D_size = 0;
  Size textureCM_size = 0;
  Size downloader_size = 0;
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
