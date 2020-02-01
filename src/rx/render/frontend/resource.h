#ifndef RX_RENDER_FRONTEND_RESOURCE_H
#define RX_RENDER_FRONTEND_RESOURCE_H
#include "rx/core/concepts/no_copy.h"
#include "rx/core/concurrency/atomic.h"

namespace rx::render::frontend {

struct interface;

struct resource
  : concepts::no_copy
{
  enum class type {
    k_buffer,
    k_target,
    k_program,
    k_texture1D,
    k_texture2D,
    k_texture3D,
    k_textureCM
  };

  static constexpr rx_size count();

  resource(interface* _frontend, type _type);
  ~resource();

  void update_resource_usage(rx_size _bytes);

  bool release_reference();
  void acquire_reference();

  type resource_type() const;

protected:
  interface* m_frontend;

private:
  type m_resource_type;
  rx_size m_resource_usage;
  concurrency::atomic<rx_size> m_reference_count;
};

inline constexpr rx_size resource::count() {
  return static_cast<rx_size>(type::k_textureCM) + 1;
}

inline bool resource::release_reference() {
  return --m_reference_count == 0;
}

inline void resource::acquire_reference() {
  m_reference_count++;
}

inline resource::type resource::resource_type() const {
  return m_resource_type;
}

} // namespace rx::render::frontend

#endif // RX_RENDER_FRONTEND_RESOURCE_H
