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

} // namespace Rx::Texture

#endif // RX_TEXTURE_FORMAT_H