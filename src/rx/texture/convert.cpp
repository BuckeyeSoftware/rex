#include "rx/texture/convert.h"

#include "rx/core/hints/unreachable.h"
#include "rx/core/hints/no_inline.h"
#include "rx/core/hints/force_inline.h"
#include "rx/core/hints/restrict.h"
#include "rx/core/hints/assume_aligned.h"

namespace Rx::Texture {

// R => RGBA
// R => RGB
// R => BGRA
// R => BGR
static RX_HINT_NO_INLINE void r_to_rgba(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, _src += 1, dst_ += 4) {
    const Byte r{_src[0]};
    dst_[0] = r;
    dst_[1] = r;
    dst_[2] = r;
    dst_[3] = 0xff;
  }
}

static RX_HINT_NO_INLINE void r_to_rgb(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 3, _src += 1) {
    dst_[0] = _src[0];
    dst_[1] = _src[0];
    dst_[2] = _src[0];
  }
}

static const constexpr auto r_to_bgra{r_to_rgba};
static const constexpr auto r_to_bgr{r_to_rgb};

// RGB => R
// RGB => BGR
// RGB => BGRA
// RGB => RGBA
static RX_HINT_NO_INLINE void rgb_to_r(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 1, _src += 3) {
    dst_[0] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgb_to_bgr(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 3, _src += 3) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgb_to_bgra(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 4, _src += 3) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
    dst_[3] = 0xff;
  }
}

static RX_HINT_NO_INLINE void rgb_to_rgba(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 4, _src += 3) {
    dst_[0] = _src[0];
    dst_[1] = _src[1];
    dst_[2] = _src[2];
    dst_[3] = 0xff;
  }
}

// BGR => R
// BGR => RGB
// BGR => RGBA
// BGR => BGRA
static RX_HINT_NO_INLINE void bgr_to_r(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 1, _src += 3) {
    dst_[0] = _src[2];
  }
}

static const constexpr auto bgr_to_rgb{rgb_to_bgr};
static const constexpr auto bgr_to_rgba{rgb_to_bgra};
static const constexpr auto bgr_to_bgra{rgb_to_rgba};

// RGBA => R
// RGBA => RGB
// RGBA => BGR
// RGBA => BGRA
static RX_HINT_NO_INLINE void rgba_to_r(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 1, _src += 4) {
    dst_[0] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgba_to_rgb(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[0];
    dst_[1] = _src[1];
    dst_[2] = _src[2];
  }
}

static RX_HINT_NO_INLINE void rgba_to_bgr(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgba_to_bgra(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 4, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
    dst_[3] = _src[3];
  }
}

// BGRA => R
// BGRA => RGB
// BGRA => BGR
// BGRA => RGBA
static RX_HINT_NO_INLINE void bgra_to_r(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 1, _src += 4) {
    dst_[0] = _src[2];
  }
}

static RX_HINT_NO_INLINE void bgra_to_rgb(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);

  for (Size i{0}; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static const constexpr auto bgra_to_bgr{rgba_to_rgb};
static const constexpr auto bgra_to_rgba{rgba_to_bgra};

static RX_HINT_FORCE_INLINE Size bpp_for_pixel_format(PixelFormat _format) {
  switch (_format) {
  case PixelFormat::k_rgba_u8:
    return 4;
  case PixelFormat::k_bgra_u8:
    return 4;
  case PixelFormat::k_rgb_u8:
    return 3;
  case PixelFormat::k_bgr_u8:
    return 3;
  case PixelFormat::k_r_u8:
    return 1;
  }
  RX_HINT_UNREACHABLE();
}

Vector<Byte> convert(Memory::Allocator& _allocator, const Byte* _data,
                        Size _samples, PixelFormat _in_format, PixelFormat _out_format)
{
  RX_HINT_ASSUME_ALIGNED(_data, Memory::Allocator::ALIGNMENT);

  Vector<Byte> result{_allocator,
    _samples * bpp_for_pixel_format(_out_format), Utility::UninitializedTag{}};

  const Byte *RX_HINT_RESTRICT src = _data;
  Byte *RX_HINT_RESTRICT dst = result.data();

  switch (_in_format) {
  case PixelFormat::k_rgba_u8:
    switch (_out_format) {
    case PixelFormat::k_rgba_u8:
      break;
    case PixelFormat::k_bgra_u8:
      rgba_to_bgra(dst, src, _samples);
      break;
    case PixelFormat::k_rgb_u8:
      rgba_to_rgb(dst, src, _samples);
      break;
    case PixelFormat::k_bgr_u8:
      rgba_to_bgr(dst, src, _samples);
      break;
    case PixelFormat::k_r_u8:
      rgba_to_r(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::k_bgra_u8:
    switch (_out_format) {
    case PixelFormat::k_rgba_u8:
      bgra_to_rgba(dst, src, _samples);
      break;
    case PixelFormat::k_bgra_u8:
      break;
    case PixelFormat::k_rgb_u8:
      bgra_to_rgb(dst, src, _samples);
      break;
    case PixelFormat::k_bgr_u8:
      bgra_to_bgr(dst, src, _samples);
      break;
    case PixelFormat::k_r_u8:
      bgra_to_r(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::k_rgb_u8:
    switch (_out_format) {
    case PixelFormat::k_rgba_u8:
      rgb_to_rgba(dst, src, _samples);
      break;
    case PixelFormat::k_bgra_u8:
      rgb_to_bgra(dst, src, _samples);
      break;
    case PixelFormat::k_rgb_u8:
      break;
    case PixelFormat::k_bgr_u8:
      rgb_to_bgr(dst, src, _samples);
      break;
    case PixelFormat::k_r_u8:
      rgb_to_r(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::k_bgr_u8:
    switch (_out_format) {
    case PixelFormat::k_rgba_u8:
      bgr_to_rgba(dst, src, _samples);
      break;
    case PixelFormat::k_bgra_u8:
      bgr_to_bgra(dst, src, _samples);
      break;
    case PixelFormat::k_rgb_u8:
      bgr_to_rgb(dst, src, _samples);
      break;
    case PixelFormat::k_bgr_u8:
      break;
    case PixelFormat::k_r_u8:
      bgr_to_r(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::k_r_u8:
    switch (_out_format) {
    case PixelFormat::k_rgba_u8:
      r_to_rgba(dst, src, _samples);
      break;
    case PixelFormat::k_bgra_u8:
      r_to_bgra(dst, src, _samples);
      break;
    case PixelFormat::k_rgb_u8:
      r_to_rgb(dst, src, _samples);
      break;
    case PixelFormat::k_bgr_u8:
      r_to_bgr(dst, src, _samples);
      break;
    case PixelFormat::k_r_u8:
      break;
    }
    break;
  }
  return result;
}

} // namespace rx::texture
