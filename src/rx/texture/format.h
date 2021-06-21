#ifndef RX_TEXTURE_FORMAT_H
#define RX_TEXTURE_FORMAT_H
#include "rx/core/types.h"

namespace Rx::Texture {

enum class PixelFormat : Uint8 {
  // Linear byte formats.
  R_U8,
  RGB_U8,
  RGBA_U8,
  BGR_U8,
  BGRA_U8,

  // sRGB byte formats.
  SRGB_U8,
  SRGBA_U8,

  // Linear float formats.
  RGBA_F32
};

enum class FileFormat {
  PNG,
  TGA,
  HDR,
  JPG
};

constexpr Size bits_per_pixel(PixelFormat _format) {
  switch (_format) {
  case PixelFormat::R_U8:
    return 8;
  case PixelFormat::RGB_U8:
    [[fallthrough]];
  case PixelFormat::BGR_U8:
    [[fallthrough]];
  case PixelFormat::SRGB_U8:
    return 3 * 8;
  case PixelFormat::RGBA_U8:
    [[fallthrough]];
  case PixelFormat::BGRA_U8:
    [[fallthrough]];
  case PixelFormat::SRGBA_U8:
    return 4 * 8;
  case PixelFormat::RGBA_F32:
    return 4 * 32;
  }
  return 0;
}

} // namespace Rx::Texture

#endif // RX_TEXTURE_FORMAT_H