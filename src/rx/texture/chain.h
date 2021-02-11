#ifndef RX_TEXTURE_CHAIN_H
#define RX_TEXTURE_CHAIN_H
#include "rx/texture/loader.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Texture {

struct Loader;

struct Chain {
  RX_MARK_NO_COPY(Chain);

  struct Level {
    Size offset;
    Size size;
    Math::Vec2z dimensions;
  };

  constexpr Chain();
  constexpr Chain(Memory::Allocator& _allocator);
  Chain(Chain&& chain_);

  Chain& operator=(Chain&& chain_);

  bool generate(Loader&& loader_, bool _has_mipchain, bool _want_mipchain);
  bool generate(Loader&& loader_, PixelFormat _want_format,
    bool _has_mipchain, bool _want_mipchain);

  bool generate(LinearBuffer&& data_, PixelFormat _has_format,
    PixelFormat _want_format, const Math::Vec2z& _dimensions,
    bool _has_mipchain, bool _want_mipchain);

  bool generate(const Byte* _data, PixelFormat _has_format,
    PixelFormat _want_format, const Math::Vec2z& _dimensions,
    bool _has_mipchain, bool _want_mipchain);

  [[nodiscard]] bool resize(const Math::Vec2z& _dimensions);

  LinearBuffer&& data();
  const LinearBuffer& data() const;

  Vector<Level>&& levels();
  const Vector<Level>& levels() const;
  const Math::Vec2z& dimensions() const;
  PixelFormat format() const;
  Size bpp() const;

  constexpr Memory::Allocator& allocator() const;

private:
  bool generate_mipchain(bool _has_mipchain, bool _want_mipchain);

  Memory::Allocator* m_allocator;
  LinearBuffer m_data;
  Vector<Level> m_levels;
  Math::Vec2z m_dimensions;
  union {
    struct {} m_nat;
    PixelFormat m_pixel_format;
  };
};

inline constexpr Chain::Chain()
  : Chain{Memory::SystemAllocator::instance()}
{
}

inline constexpr Chain::Chain(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_data{allocator()}
  , m_levels{allocator()}
  , m_nat{}
{
}

inline Chain::Chain(Chain&& chain_)
  : m_allocator{chain_.m_allocator}
  , m_data{Utility::move(chain_.m_data)}
  , m_levels{Utility::move(chain_.m_levels)}
  , m_dimensions{Utility::exchange(chain_.m_dimensions, Math::Vec2z{})}
  , m_pixel_format{chain_.m_pixel_format}
{
}

inline Chain& Chain::operator=(Chain&& chain_) {
  RX_ASSERT(&chain_ != this, "self assignment");

  m_allocator = chain_.m_allocator;
  m_data = Utility::move(chain_.m_data);
  m_levels = Utility::move(chain_.m_levels);
  m_dimensions = Utility::exchange(chain_.m_dimensions, Math::Vec2z{});
  m_pixel_format = chain_.m_pixel_format;

  return *this;
}

inline bool Chain::generate(Loader&& loader_, bool _has_mipchain,
  bool _want_mipchain)
{
  const PixelFormat format = loader_.format();
  return generate(Utility::move(loader_.data()), format, format,
    loader_.dimensions(), _has_mipchain, _want_mipchain);
}

inline bool Chain::generate(Loader&& loader_, PixelFormat _want_format,
  bool _has_mipchain, bool _want_mipchain)
{
  return generate(Utility::move(loader_.data()), loader_.format(), _want_format,
    loader_.dimensions(), _has_mipchain, _want_mipchain);
}

inline LinearBuffer&& Chain::data() {
  return Utility::move(m_data);
}

inline const LinearBuffer& Chain::data() const {
  return m_data;
}

inline Vector<Chain::Level>&& Chain::levels() {
  return Utility::move(m_levels);
}

inline const Vector<Chain::Level>& Chain::levels() const {
  return m_levels;
}

inline const Math::Vec2z& Chain::dimensions() const {
  return m_dimensions;
}

inline PixelFormat Chain::format() const {
  return m_pixel_format;
}

inline Size Chain::bpp() const {
  switch (m_pixel_format) {
  case PixelFormat::R_U8:
    return 1;
  case PixelFormat::BGR_U8:
    [[fallthrough]];
  case PixelFormat::RGB_U8:
    return 3;
  case PixelFormat::BGRA_U8:
    [[fallthrough]];
  case PixelFormat::RGBA_U8:
    return 4;
  case PixelFormat::RGBA_F32:
    // TODO(dweiler): Support floating point formats in chain.
    break;
  case PixelFormat::SRGB_U8:
    return 3;
  case PixelFormat::SRGBA_U8:
    return 4;
  }

  return 0;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Chain::allocator() const {
  return *m_allocator;
}

} // namespace Rx::Texture

#endif // RX_TEXTURE_TEXTURE_H
