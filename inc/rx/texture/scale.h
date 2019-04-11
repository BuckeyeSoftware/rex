#ifndef RX_TEXTURE_SCALE_H
#define RX_TEXTURE_SCALE_H

#include <rx/core/types.h>

namespace rx::texture {

// can be called with C=[1..4]
template<rx_size C>
void halve(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_);

// can be called with C=[1..4]
template<rx_size C>
void shift(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh);

// can be called with C=[1..4]
template<rx_size C>
void scale(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _stride,
  rx_byte* dst_, rx_size _dw, rx_size _dh);

// "safe" routing scaling function, calls the appropriate functions as defined
// above based on _bpp and _stride
void scale(const rx_byte* _src, rx_size _sw, rx_size _sh, rx_size _bpp,
  rx_size _stride, rx_byte* dst_, rx_size _dw, rx_size _dh);

} // namespace rx::texture

#endif // RX_TEXTURE_SCALE_H