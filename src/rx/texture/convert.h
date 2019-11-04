#ifndef RX_TEXTURE_CONVERT_H
#define RX_TEXTURE_CONVERT_H
#include "rx/texture/loader.h"

namespace rx::texture {

vector<rx_byte> convert(memory::allocator* _allocator,
  const vector<rx_byte>& _data, pixel_format _in_format,
  pixel_format _out_format);

} // namespace rx::texture

#endif // RX_TEXTURE_CONVERT_H
