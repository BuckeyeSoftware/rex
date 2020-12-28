#ifndef RX_TEXTURE_SCALE_H
#define RX_TEXTURE_SCALE_H
#include "rx/core/types.h"
#include "rx/core/hints/restrict.h"

namespace Rx::Texture {

// can be called with C=[1..4]
template<Size C>
void halve(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _stride, Byte *RX_HINT_RESTRICT dst_);

// can be called with C=[1..4]
template<Size C>
void shift(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw, Size _dh);

// can be called with C=[1..4]
template<Size C>
void scale(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw, Size _dh);

// "safe" routing scaling function, calls the appropriate functions as defined
// above based on _bpp and _stride
void scale(const Byte *RX_HINT_RESTRICT _src, Size _sw, Size _sh,
  Size _bpp, Size _stride, Byte *RX_HINT_RESTRICT dst_, Size _dw,
  Size _dh);

} // namespace Rx::Texture

#endif // RX_TEXTURE_SCALE_H
