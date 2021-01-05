#ifndef RX_TEXTURE_LOADER_H
#define RX_TEXTURE_LOADER_H
#include "rx/core/linear_buffer.h"
#include "rx/core/string.h"

#include "rx/core/hints/unreachable.h"

#include "rx/math/vec2.h"

namespace Rx {
  struct Stream;
}

namespace Rx::Texture {

enum class PixelFormat : Uint8 {
  RGBA_U8,
  BGRA_U8,
  RGB_U8,
  BGR_U8,
  R_U8
};

struct Loader {
  RX_MARK_NO_COPY(Loader);
  RX_MARK_NO_MOVE(Loader);

  constexpr Loader();
  constexpr Loader(Memory::Allocator& _allocator);
  ~Loader() = default;

  bool load(Stream* _stream, PixelFormat _want_format,
    const Math::Vec2z& _max_dimensions);
  bool load(const String& _file_name, PixelFormat _want_format,
    const Math::Vec2z& _max_dimensions);

  Size bpp() const;
  Size channels() const;
  const Math::Vec2z& dimensions() const &;
  LinearBuffer&& data();
  PixelFormat format() const;

  constexpr Memory::Allocator& allocator() const;

private:
  Memory::Allocator& m_allocator;
  LinearBuffer m_data;
  Size m_bpp;
  Size m_channels;
  Math::Vec2z m_dimensions;
};

inline constexpr Loader::Loader()
  : Loader{Memory::SystemAllocator::instance()}
{
}

inline constexpr Loader::Loader(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_data{allocator()}
  , m_bpp{0}
  , m_channels{0}
  , m_dimensions{}
{
}

inline Size Loader::bpp() const {
  return m_bpp;
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
  switch (m_bpp) {
  case 4:
    return PixelFormat::RGBA_U8;
  case 3:
    return PixelFormat::RGB_U8;
  case 1:
    return PixelFormat::R_U8;
  }
  RX_HINT_UNREACHABLE();
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Loader::allocator() const {
  return m_allocator;
}

} // namespace Rx::Texture

#endif // RX_TEXTURE_LOADER_H
