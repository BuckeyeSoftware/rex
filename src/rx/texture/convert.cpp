#include "rx/texture/convert.h"

#include "rx/core/hints/unreachable.h"
#include "rx/core/hints/no_inline.h"
#include "rx/core/hints/force_inline.h"
#include "rx/core/hints/restrict.h"
#include "rx/core/hints/assume_aligned.h"

namespace rx::texture {

// R => RGBA
// R => RGB
// R => BGRA
// R => BGR
static RX_HINT_NO_INLINE void r_to_rgba(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, _src += 1, dst_ += 4) {
    const rx_byte r{_src[0]};
    dst_[0] = r;
    dst_[1] = r;
    dst_[2] = r;
    dst_[3] = 0xff;
  }
}

static RX_HINT_NO_INLINE void r_to_rgb(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 3, _src += 1) {
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
static RX_HINT_NO_INLINE void rgb_to_r(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 1, _src += 3) {
    dst_[0] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgb_to_bgr(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 3, _src += 3) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgb_to_bgra(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 4, _src += 3) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
    dst_[3] = 0xff;
  }
}

static RX_HINT_NO_INLINE void rgb_to_rgba(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 4, _src += 3) {
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
static RX_HINT_NO_INLINE void bgr_to_r(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 1, _src += 3) {
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
static RX_HINT_NO_INLINE void rgba_to_r(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 1, _src += 4) {
    dst_[0] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgba_to_rgb(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[0];
    dst_[1] = _src[1];
    dst_[2] = _src[2];
  }
}

static RX_HINT_NO_INLINE void rgba_to_bgr(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static RX_HINT_NO_INLINE void rgba_to_bgra(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 4, _src += 4) {
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
static RX_HINT_NO_INLINE void bgra_to_r(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 1, _src += 4) {
    dst_[0] = _src[2];
  }
}

static RX_HINT_NO_INLINE void bgra_to_rgb(rx_byte *RX_HINT_RESTRICT dst_,
  const rx_byte *RX_HINT_RESTRICT _src, rx_size _samples)
{
  RX_HINT_ASSUME_ALIGNED(dst_, memory::allocator::k_alignment);
  RX_HINT_ASSUME_ALIGNED(_src, memory::allocator::k_alignment);

  for (rx_size i{0}; i < _samples; i++, dst_ += 3, _src += 4) {
    dst_[0] = _src[2];
    dst_[1] = _src[1];
    dst_[2] = _src[0];
  }
}

static const constexpr auto bgra_to_bgr{rgba_to_rgb};
static const constexpr auto bgra_to_rgba{rgba_to_bgra};

static RX_HINT_FORCE_INLINE rx_size bpp_for_pixel_format(pixel_format _format) {
  switch (_format) {
  case pixel_format::k_rgba_u8:
    return 4;
  case pixel_format::k_bgra_u8:
    return 4;
  case pixel_format::k_rgb_u8:
    return 3;
  case pixel_format::k_bgr_u8:
    return 3;
  case pixel_format::k_r_u8:
    return 1;
  }
  RX_HINT_UNREACHABLE();
}

vector<rx_byte> convert(memory::allocator* _allocator, const rx_byte* _data,
  rx_size _samples, pixel_format _in_format, pixel_format _out_format)
{
  RX_HINT_ASSUME_ALIGNED(_data, memory::allocator::k_alignment);

  vector<rx_byte> result{_allocator, _samples * bpp_for_pixel_format(_out_format)};
  const rx_byte *RX_HINT_RESTRICT src{_data};
  rx_byte *RX_HINT_RESTRICT dst{result.data()};

  switch (_in_format) {
  case pixel_format::k_rgba_u8:
    switch (_out_format) {
    case pixel_format::k_rgba_u8:
      break;
    case pixel_format::k_bgra_u8:
      rgba_to_bgra(dst, src, _samples);
      break;
    case pixel_format::k_rgb_u8:
      rgba_to_rgb(dst, src, _samples);
      break;
    case pixel_format::k_bgr_u8:
      rgba_to_bgr(dst, src, _samples);
      break;
    case pixel_format::k_r_u8:
      rgba_to_r(dst, src, _samples);
      break;
    }
    break;
  case pixel_format::k_bgra_u8:
    switch (_out_format) {
    case pixel_format::k_rgba_u8:
      bgra_to_rgba(dst, src, _samples);
      break;
    case pixel_format::k_bgra_u8:
      break;
    case pixel_format::k_rgb_u8:
      bgra_to_rgb(dst, src, _samples);
      break;
    case pixel_format::k_bgr_u8:
      bgra_to_bgr(dst, src, _samples);
      break;
    case pixel_format::k_r_u8:
      bgra_to_r(dst, src, _samples);
      break;
    }
    break;
  case pixel_format::k_rgb_u8:
    switch (_out_format) {
    case pixel_format::k_rgba_u8:
      rgb_to_rgba(dst, src, _samples);
      break;
    case pixel_format::k_bgra_u8:
      rgb_to_bgra(dst, src, _samples);
      break;
    case pixel_format::k_rgb_u8:
      break;
    case pixel_format::k_bgr_u8:
      rgb_to_bgr(dst, src, _samples);
      break;
    case pixel_format::k_r_u8:
      rgb_to_r(dst, src, _samples);
      break;
    }
    break;
  case pixel_format::k_bgr_u8:
    switch (_out_format) {
    case pixel_format::k_rgba_u8:
      bgr_to_rgba(dst, src, _samples);
      break;
    case pixel_format::k_bgra_u8:
      bgr_to_bgra(dst, src, _samples);
      break;
    case pixel_format::k_rgb_u8:
      bgr_to_rgb(dst, src, _samples);
      break;
    case pixel_format::k_bgr_u8:
      break;
    case pixel_format::k_r_u8:
      bgr_to_r(dst, src, _samples);
      break;
    }
    break;
  case pixel_format::k_r_u8:
    switch (_out_format) {
    case pixel_format::k_rgba_u8:
      r_to_rgba(dst, src, _samples);
      break;
    case pixel_format::k_bgra_u8:
      r_to_bgra(dst, src, _samples);
      break;
    case pixel_format::k_rgb_u8:
      r_to_rgb(dst, src, _samples);
      break;
    case pixel_format::k_bgr_u8:
      r_to_bgr(dst, src, _samples);
      break;
    case pixel_format::k_r_u8:
      break;
    }
    break;
  }
  return result;
}

} // namespace rx::texture
