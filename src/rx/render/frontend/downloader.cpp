#include "rx/render/frontend/downloader.h"
#include "rx/render/frontend/context.h"

namespace Rx::Render::Frontend {

Downloader::Downloader(Context* _context)
  : Resource{_context, Resource::Type::k_downloader}
  , m_flags{0}
  , m_buffers{0}
  , m_pixels{_context->allocator()}
  , m_downloads{0}
  , m_latency{0}
  , m_count{0}
{
}

void Downloader::record_format(Texture2D::DataFormat _data_format) {
  RX_ASSERT(!(m_flags & DATA_FORMAT), "already recorded format");
  m_data_format = _data_format;
  m_flags |= DATA_FORMAT;
}

void Downloader::record_dimensions(const Math::Vec2z& _dimensions) {
  RX_ASSERT(!(m_flags & DIMENSIONS), "already recorded dimensions");

  RX_ASSERT(m_flags & DATA_FORMAT, "data format not recorded");
  RX_ASSERT(m_flags & BUFFERS, "buffers not recorded");

  const Size bytes = _dimensions.area() * Texture::bits_per_pixel(m_data_format) / 8;
  RX_ASSERT(m_pixels.resize(bytes), "out of memory");
  m_dimensions = _dimensions;
  m_flags |= DIMENSIONS;

  // Only an estimation of memory usage.
  update_resource_usage(m_pixels.size() * m_buffers);
}

void Downloader::record_buffers(Size _buffers) {
  RX_ASSERT(!(m_flags & BUFFERS), "already recorded buffers");
  m_buffers = _buffers;
  m_flags |= BUFFERS;
}

} // namespace Rx::Render::Frontend
