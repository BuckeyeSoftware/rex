#ifndef RX_TEXTURE_LOADER_H
#define RX_TEXTURE_LOADER_H
#include "rx/texture/format.h"

#include "rx/core/linear_buffer.h"

#include "rx/math/vec2.h"

namespace Rx {
  struct String;
  struct StringView;
}

namespace Rx::Stream {
  struct UntrackedStream;
}

namespace Rx::Texture {

// Checks if the PixelFormat |_format| has an alpha channel.
inline bool has_alpha_channel(PixelFormat _format) {
  return _format == PixelFormat::RGBA_U8 ||
         _format == PixelFormat::BGRA_U8 ||
         _format == PixelFormat::SRGBA_U8 ||
         _format == PixelFormat::RGBA_F32;
}

// Checks if the PixelFormat is an sRGB format.
inline bool is_srgb_format(PixelFormat _format) {
  return _format == PixelFormat::SRGB_U8 ||
         _format == PixelFormat::SRGBA_U8;
}

// Checks if the PixelFormat is a float format.
inline bool is_float_format(PixelFormat _format) {
  return _format == PixelFormat::RGBA_F32;
}

struct Loader {
  RX_MARK_NO_COPY(Loader);
  RX_MARK_NO_MOVE(Loader);

  constexpr Loader(Memory::Allocator& _allocator);
  ~Loader() = default;

  [[nodiscard]] bool load(Stream::UntrackedStream& _stream, PixelFormat _want_format,
    const Math::Vec2z& _max_dimensions);
  [[nodiscard]] bool load(const StringView& _file_name, PixelFormat _want_format,
    const Math::Vec2z& _max_dimensions);

  Size bits_per_pixel() const;
  Size channels() const;
  const Math::Vec2z& dimensions() const &;
  LinearBuffer&& data();
  PixelFormat format() const;

  constexpr Memory::Allocator& allocator() const;

private:
  Memory::Allocator& m_allocator;
  LinearBuffer m_data;
  Size m_channels;
  PixelFormat m_format;
  Math::Vec2z m_dimensions;
};

inline constexpr Loader::Loader(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{allocator()}
  , m_channels{0}
  , m_dimensions{}
{
}

inline Size Loader::channels() const {
  return m_channels;
}

inline const Math::Vec2z& Loader::dimensions() const & {
  return m_dimensions;
}

inline LinearBuffer&& Loader::data() {
  return Utility::move(m_data);
}

inline PixelFormat Loader::format() const {
  return m_format;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Loader::allocator() const {
  return m_allocator;
}

} // namespace Rx::Texture

#endif // RX_TEXTURE_LOADER_H
