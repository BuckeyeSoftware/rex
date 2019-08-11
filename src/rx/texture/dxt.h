#ifndef RX_TEXTURE_DXT_H
#define RX_TEXTURE_DXT_H
#include "rx/core/array.h"

namespace rx::texture {

enum class dxt_type {
  k_dxt1,
  k_dxt5
};

template<dxt_type T>
array<rx_byte> dxt_compress(memory::allocator* _allocator,
  const rx_byte *const _uncompressed, rx_size _width, rx_size _height,
  rx_size _channels, rx_size& out_size_, rx_size& optimized_blocks_);

} // namespace rx::texture

#endif // RX_TEXTURE_DXT_H