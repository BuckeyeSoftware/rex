#ifndef RX_CORE_UTILITY_TRANSMUTE_H
#define RX_CORE_UTILITY_TRANSMUTE_H
#include "rx/core/types.h"

namespace Rx::Utility {

inline void write_u32(Byte* dst_, Uint32 _value) {
  dst_[0] = _value & 0xffu;
  dst_[1] = (_value & 0xff00u) >> 8;
  dst_[2] = (_value & 0xff0000u) >> 16;
  dst_[3] = (_value & 0xff000000u) >> 24;
}

inline Uint32 read_u32(const Byte* _src) {
  Uint32 result = 0;
  result |= 0xffu & Uint32(_src[0]);
  result |= (0xffu & Uint32(_src[1])) << 8;
  result |= (0xffu & Uint32(_src[2])) << 16;
  result |= (0xffu & Uint32(_src[3])) << 24;
  return result;
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_TRANSMUTE_H