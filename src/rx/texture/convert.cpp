#include "rx/texture/convert.h"

#include "rx/core/hints/unreachable.h"
#include "rx/core/hints/no_inline.h"
#include "rx/core/hints/force_inline.h"
#include "rx/core/hints/restrict.h"
#include "rx/core/hints/assume_aligned.h"

namespace Rx::Texture {

// R_U8 => RGBA_U8
// R_U8 => RGB_U8
// R_U8 => BGRA_U8
// R_U8 => BGR_U8
// R_U8 => RGBA_F32
static RX_HINT_NO_INLINE void r_u8_to_rgba_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, _src += 1, dst_ += 4) {
    const auto r = _src[0];
    dst_[0] = r;
    dst_[1] = r;
    dst_[2] = r;
    dst_[3] = 0xff;
  }
}

static RX_HINT_NO_INLINE void r_u8_to_rgb_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 3, _src += 1) {
    dst_[0] = _src[0];
    dst_[1] = _src[0];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void r_u8_to_rgba_f32(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto dst = reinterpret_cast<Float32*>(dst_);
  for (Size i = 0; i < _samples; i++, dst += 4, _src += 1) {
    const auto luma = Float32(_src[0]) / 255.0f;
    dst[0] = luma;
    dst[1] = luma;
    dst[2] = luma;
    dst[3] = 1.0f;
  }
}

static inline const constexpr auto r_u8_to_bgra_u8 = r_u8_to_rgba_u8;
static inline const constexpr auto r_u8_to_bgr_u8  = r_u8_to_rgb_u8;

// RGB_U8 => R_U8
// RGB_U8 => BGR_U8
// RGB_U8 => BGRA_U8
// RGB_U8 => RGBA_U8
// RGB_U8 => RGBA_F32
static RX_HINT_NO_INLINE void rgb_u8_to_r_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 1, _src += 3) {
    dst_[0] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgb_u8_to_bgr_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 3, _src += 3) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgb_u8_to_bgra_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 4, _src += 3) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
    dst_[3] = 0xff;
  }
}

static RX_HINT_NO_INLINE void rgb_u8_to_rgba_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 4, _src += 3) {
    dst_[0] = _src[0];
    dst_[1] = _src[1];
    dst_[2] = _src[2];
    dst_[3] = 0xff;
  }
}

static RX_HINT_NO_INLINE void rgb_u8_to_rgba_f32(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto dst = reinterpret_cast<Float32*>(dst_);
  for (Size i = 0; i < _samples; i++, dst += 4, _src += 3) {
    dst[0] = Float32(_src[0]) / 255.0f;
    dst[1] = Float32(_src[1]) / 255.0f;
    dst[2] = Float32(_src[2]) / 255.0f;
    dst[3] = 1.0f;
  }
}

// BGR_U8 => R_U8
// BGR_U8 => RGB_U8
// BGR_U8 => RGBA_U8
// BGR_U8 => BGRA_U8
// BGR_U8 => RGBA_F32
static RX_HINT_NO_INLINE void bgr_u8_to_r_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 1, _src += 3) {
    dst_[0] = _src[2];
  }
}

static RX_HINT_NO_INLINE void bgr_u8_to_rgba_f32(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto dst = reinterpret_cast<Float32*>(dst_);
  for (Size i = 0; i < _samples; i++, dst += 4, _src += 3) {
    dst[0] = Float32(_src[2]) / 255.0f;
    dst[1] = Float32(_src[1]) / 255.0f;
    dst[2] = Float32(_src[0]) / 255.0f;
    dst[3] = 1.0f;
  }
}

static inline const constexpr auto bgr_u8_to_rgb_u8  = rgb_u8_to_bgr_u8;
static inline const constexpr auto bgr_u8_to_rgba_u8 = rgb_u8_to_bgra_u8;
static inline const constexpr auto bgr_u8_to_bgra_u8 = rgb_u8_to_rgba_u8;

// RGBA_U8 => R_U8
// RGBA_U8 => RGB_U8
// RGBA_U8 => BGR_U8
// RGBA_U8 => BGRA_U8
// RGBA_U8 => RGBA_F32
static RX_HINT_NO_INLINE void rgba_u8_to_r_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 1, _src += 4) {
    dst_[0] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgba_u8_to_rgb_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[0];
    dst_[1] = _src[1];
    dst_[2] = _src[2];
  }
}

static RX_HINT_NO_INLINE void rgba_u8_to_bgr_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgba_u8_to_bgra_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 4, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
    dst_[3] = _src[3];
  }
}

static RX_HINT_NO_INLINE void rgba_u8_to_rgba_f32(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto dst = reinterpret_cast<Float32*>(dst_);
  for (Size i = 0; i < _samples; i++, dst += 4, _src += 3) {
    dst[0] = Float32(_src[0]) / 255.0f;
    dst[1] = Float32(_src[1]) / 255.0f;
    dst[2] = Float32(_src[2]) / 255.0f;
    dst[3] = Float32(_src[3]) / 255.0f;
  }
}

// BGRA_U8 => R_U8
// BGRA_U8 => RGB_U8
// BGRA_U8 => BGR_U8
// BGRA_U8 => RGBA_U8
// BGRA_U8 => RGBA_F32
static RX_HINT_NO_INLINE void bgra_u8_to_r_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 1, _src += 4) {
    dst_[0] = _src[2];
  }
}

static RX_HINT_NO_INLINE void bgra_u8_to_rgb_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  for (Size i = 0; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void bgra_u8_to_rgba_f32(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto dst = reinterpret_cast<Float32*>(dst_);
  for (Size i = 0; i < _samples; i++, dst += 4, _src += 3) {
    dst[0] = Float32(_src[2]) / 255.0f;
    dst[1] = Float32(_src[1]) / 255.0f;
    dst[2] = Float32(_src[0]) / 255.0f;
    dst[3] = Float32(_src[3]) / 255.0f;
  }
}

static inline const constexpr auto bgra_u8_to_bgr_u8  = rgba_u8_to_rgb_u8;
static inline const constexpr auto bgra_u8_to_rgba_u8 = rgba_u8_to_bgra_u8;

// RGBA_F32 => R_U8
// RGBA_F32 => RGB_U8
// RGBA_F32 => BGR_U8
// RGBA_F32 => RGBA_U8
// RGBA_F32 => BGRA_U8
static RX_HINT_NO_INLINE void rgba_f32_to_r_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto src = reinterpret_cast<const Float32*>(_src);
  for (Size i = 0; i < _samples; i++, dst_ += 1, src += 4) {
    dst_[0] = Byte(src[0] * 255.0f);
  }
}

static RX_HINT_NO_INLINE void rgba_f32_to_rgb_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto src = reinterpret_cast<const Float32*>(_src);
  for (Size i = 0; i < _samples; i++, dst_ += 3, src += 4) {
    dst_[0] = Byte(src[0] * 255.0f);
    dst_[1] = Byte(src[1] * 255.0f);
    dst_[2] = Byte(src[2] * 255.0f);
  }
}

static RX_HINT_NO_INLINE void rgba_f32_to_bgr_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto src = reinterpret_cast<const Float32*>(_src);
  for (Size i = 0; i < _samples; i++, dst_ += 3, src += 4) {
    dst_[0] = Byte(src[2] * 255.0f);
    dst_[1] = Byte(src[1] * 255.0f);
    dst_[2] = Byte(src[0] * 255.0f);
  }
}

static RX_HINT_NO_INLINE void rgba_f32_to_rgba_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto src = reinterpret_cast<const Float32*>(_src);
  for (Size i = 0; i < _samples; i++, dst_ += 4, src += 4) {
    dst_[0] = Byte(src[0] * 255.0f);
    dst_[1] = Byte(src[1] * 255.0f);
    dst_[2] = Byte(src[2] * 255.0f);
    dst_[3] = Byte(src[3] * 255.0f);
  }
}

static RX_HINT_NO_INLINE void rgba_f32_to_bgra_u8(Byte *RX_HINT_RESTRICT dst_,
  const Byte *RX_HINT_RESTRICT _src, Size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, Memory::Allocator::ALIGNMENT);
  RX_HINT_ASSUME_ALIGNED(_src, Memory::Allocator::ALIGNMENT);
  auto src = reinterpret_cast<const Float32*>(_src);
  for (Size i = 0; i < _samples; i++, dst_ += 4, src += 4) {
    dst_[0] = Byte(src[2] * 255.0f);
    dst_[1] = Byte(src[1] * 255.0f);
    dst_[2] = Byte(src[0] * 255.0f);
    dst_[3] = Byte(src[3] * 255.0f);
  }
}

static RX_HINT_FORCE_INLINE Size bpp_for_pixel_format(PixelFormat _format) {
  switch (_format) {
  case PixelFormat::RGBA_U8:
    return 4;
  case PixelFormat::BGRA_U8:
    return 4;
  case PixelFormat::RGB_U8:
    return 3;
  case PixelFormat::BGR_U8:
    return 3;
  case PixelFormat::R_U8:
    return 1;
  case PixelFormat::RGBA_F32:
    return 4 * 4;
  }
  RX_HINT_UNREACHABLE();
}

Optional<LinearBuffer> convert(
  Memory::Allocator& _allocator, const Byte* _data, Size _samples,
  PixelFormat _in_format, PixelFormat _out_format)
{
  RX_HINT_ASSUME_ALIGNED(_data, Memory::Allocator::ALIGNMENT);

  LinearBuffer result{_allocator};
  if (!result.resize(_samples * bpp_for_pixel_format(_out_format))) {
    return nullopt;
  }

  const Byte *RX_HINT_RESTRICT src = _data;
  Byte *RX_HINT_RESTRICT dst = result.data();

  switch (_in_format) {
  case PixelFormat::RGBA_U8:
    switch (_out_format) {
    case PixelFormat::RGBA_U8:
      break;
    case PixelFormat::BGRA_U8:
      rgba_u8_to_bgra_u8(dst, src, _samples);
      break;
    case PixelFormat::RGB_U8:
      rgba_u8_to_rgb_u8(dst, src, _samples);
      break;
    case PixelFormat::BGR_U8:
      rgba_u8_to_bgr_u8(dst, src, _samples);
      break;
    case PixelFormat::R_U8:
      rgba_u8_to_r_u8(dst, src, _samples);
      break;
    case PixelFormat::RGBA_F32:
      rgba_u8_to_rgba_f32(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::BGRA_U8:
    switch (_out_format) {
    case PixelFormat::RGBA_U8:
      bgra_u8_to_rgba_u8(dst, src, _samples);
      break;
    case PixelFormat::BGRA_U8:
      break;
    case PixelFormat::RGB_U8:
      bgra_u8_to_rgb_u8(dst, src, _samples);
      break;
    case PixelFormat::BGR_U8:
      bgra_u8_to_bgr_u8(dst, src, _samples);
      break;
    case PixelFormat::R_U8:
      bgra_u8_to_r_u8(dst, src, _samples);
      break;
    case PixelFormat::RGBA_F32:
      bgra_u8_to_rgba_f32(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::RGB_U8:
    switch (_out_format) {
    case PixelFormat::RGBA_U8:
      rgb_u8_to_rgba_u8(dst, src, _samples);
      break;
    case PixelFormat::BGRA_U8:
      rgb_u8_to_bgra_u8(dst, src, _samples);
      break;
    case PixelFormat::RGB_U8:
      break;
    case PixelFormat::BGR_U8:
      rgb_u8_to_bgr_u8(dst, src, _samples);
      break;
    case PixelFormat::R_U8:
      rgb_u8_to_r_u8(dst, src, _samples);
      break;
    case PixelFormat::RGBA_F32:
      rgb_u8_to_rgba_f32(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::BGR_U8:
    switch (_out_format) {
    case PixelFormat::RGBA_U8:
      bgr_u8_to_rgba_u8(dst, src, _samples);
      break;
    case PixelFormat::BGRA_U8:
      bgr_u8_to_bgra_u8(dst, src, _samples);
      break;
    case PixelFormat::RGB_U8:
      bgr_u8_to_rgb_u8(dst, src, _samples);
      break;
    case PixelFormat::BGR_U8:
      break;
    case PixelFormat::R_U8:
      bgr_u8_to_r_u8(dst, src, _samples);
      break;
    case PixelFormat::RGBA_F32:
      bgr_u8_to_rgba_f32(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::R_U8:
    switch (_out_format) {
    case PixelFormat::RGBA_U8:
      r_u8_to_rgba_u8(dst, src, _samples);
      break;
    case PixelFormat::BGRA_U8:
      r_u8_to_bgra_u8(dst, src, _samples);
      break;
    case PixelFormat::RGB_U8:
      r_u8_to_rgb_u8(dst, src, _samples);
      break;
    case PixelFormat::BGR_U8:
      r_u8_to_bgr_u8(dst, src, _samples);
      break;
    case PixelFormat::R_U8:
      break;
    case PixelFormat::RGBA_F32:
      r_u8_to_rgba_f32(dst, src, _samples);
      break;
    }
    break;
  case PixelFormat::RGBA_F32:
    switch (_out_format) {
    case PixelFormat::RGBA_U8:
      rgba_f32_to_rgba_u8(dst, src, _samples);
      break;
    case PixelFormat::BGRA_U8:
      rgba_f32_to_bgra_u8(dst, src, _samples);
      break;
    case PixelFormat::RGB_U8:
      rgba_f32_to_rgb_u8(dst, src, _samples);
      break;
    case PixelFormat::BGR_U8:
      rgba_f32_to_bgr_u8(dst, src, _samples);
      break;
    case PixelFormat::R_U8:
      rgba_f32_to_r_u8(dst, src, _samples);
      break;
    case PixelFormat::RGBA_F32:
      break;
    }
  }
  return result;
}

} // namespace Rx::Texture
