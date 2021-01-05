#ifndef RX_TEXTURE_DXT_H
#define RX_TEXTURE_DXT_H
#include "rx/core/linear_buffer.h"

namespace Rx::Texture {

enum class DXTType {
  DXT1,
  DXT5
};

template<DXTType T>
Optional<LinearBuffer> dxt_compress(
  Memory::Allocator& _allocator, const Byte *const _uncompressed, Size _width,
  Size _height, Size _channels, Size& out_size_, Size& optimized_blocks_);

} // namespace Rx::Texture

#endif // RX_TEXTURE_DXT_H
