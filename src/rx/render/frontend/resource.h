#ifndef RX_RENDER_FRONTEND_RESOURCE_H
#define RX_RENDER_FRONTEND_RESOURCE_H
#include "rx/core/concurrency/atomic.h"
#include "rx/core/markers.h"

namespace Rx::Render::Frontend {

struct Context;

struct Resource {
  RX_MARK_NO_COPY(Resource);

  enum class Type : Uint8 {
    BUFFER,
    TARGET,
    PROGRAM,
    TEXTURE1D,
    TEXTURE2D,
    TEXTURE3D,
    TEXTURECM,
    DOWNLOADER
  };

  static constexpr Size count();

  Resource(Context* _frontend, Type _type);
  ~Resource();

  void update_resource_usage(Size _bytes);

  bool release_reference();
  void acquire_reference();

  Type resource_type() const;
  Size resource_usage() const;

protected:
  Context* m_frontend;

private:
  Type m_resource_type;
  Size m_resource_usage;
  Concurrency::Atomic<Size> m_reference_count;
};

inline constexpr Size Resource::count() {
  return static_cast<Size>(Type::DOWNLOADER) + 1;
}

inline bool Resource::release_reference() {
  return --m_reference_count == 0;
}

inline void Resource::acquire_reference() {
  m_reference_count++;
}

inline Resource::Type Resource::resource_type() const {
  return m_resource_type;
}

inline Size Resource::resource_usage() const {
  return m_resource_usage;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_RESOURCE_H
