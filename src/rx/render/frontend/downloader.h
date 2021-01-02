#ifndef RX_RENDER_FRONTEND_DOWNLOADER_H
#define RX_RENDER_FRONTEND_DOWNLOADER_H
#include "rx/render/frontend/texture.h"

namespace Rx::Render::Frontend {

struct Downloader
  : Resource
{
  RX_MARK_NO_COPY(Downloader);
  RX_MARK_NO_MOVE(Downloader);

  Downloader(Context* _context);

  void record_format(Texture2D::DataFormat _data_format);
  void record_dimensions(const Math::Vec2z& _dimensions);
  void record_buffers(Size _buffers);

  bool is_ready() const;

  Texture2D::DataFormat format() const;
  const Math::Vec2z& dimensions() const;
  Size downloads() const;
  Size latency() const;
  Size buffers() const;

  const LinearBuffer& pixels() const &;

// TODO(dweiler): Find out a way for the backend to update the frontend objects.
// private:
  enum : Uint32 {
    DATA_FORMAT = 1 << 0,
    DIMENSIONS  = 1 << 1,
    BUFFERS     = 1 << 2
  };

  Uint32 m_flags;
  Texture2D::DataFormat m_data_format;
  Math::Vec2z m_dimensions;
  Size m_buffers;
  LinearBuffer m_pixels;
  Concurrency::Atomic<Size> m_downloads;
  Concurrency::Atomic<Size> m_latency;
  Concurrency::Atomic<Size> m_count;
};

inline bool Downloader::is_ready() const {
  return m_downloads >= m_buffers;
}

inline Texture2D::DataFormat Downloader::format() const {
  return m_data_format;
}

inline const Math::Vec2z& Downloader::dimensions() const {
  return m_dimensions;
}

inline Size Downloader::downloads() const {
  return m_downloads;
}

inline Size Downloader::latency() const {
  return m_latency;
}

inline Size Downloader::buffers() const {
  return m_buffers;
}

inline const LinearBuffer& Downloader::pixels() const & {
  return m_pixels;
}

} // namespace Rx::Render::Frontend

#endif // RX_RENDER_FRONTEND_DOWNLOADER_H
