#ifndef RX_TEXTURE_CONVERT_H
#define RX_TEXTURE_CONVERT_H
#include "rx/texture/loader.h"
#include "rx/core/linear_buffer.h"

namespace Rx::Texture {

Optional<LinearBuffer> convert(
  Memory::Allocator& _allocator, const Byte* _data, Size _samples,
  PixelFormat _in_format, PixelFormat _out_format);

} // namespace Rx::Texture

#endif // RX_TEXTURE_CONVERT_H
