#ifndef RX_CORE_UTILITY_BYTE_SWAP_H
#define RX_CORE_UTILITY_BYTE_SWAP_H
#include "rx/core/types.h"

namespace Rx::Utility {

inline constexpr Uint16 swap_u16(Uint16 _value) {
  return (_value << 8) | (_value >> 8);
}

inline constexpr Sint16 swap_s16(Sint16 _value) {
  return (_value << 8) | ((_value >> 8) & 0xff);
}

inline constexpr Uint32 swap_u32(Uint32 _value) {
  _value = ((_value << 8) & 0xff00ff00_u32) | ((_value >> 8) & 0xff00ff_u32);
  return (_value << 16) | (_value >> 16);
}

inline constexpr Sint32 swap_s32(Sint32 _value) {
  _value = ((_value << 8) & 0xff00ff00_u32) | ((_value >> 8) & 0xff00ff_u32);
  return (_value << 16) | ((_value >> 16) & 0xffff_u32);
}

inline constexpr Uint64 swap_u64(Uint64 _value) {
  _value = ((_value << 8) & 0xff00ff00ff00ff00_u64) | ((_value >> 8) & 0x00ff00ff00ff00ff_u64);
  _value = ((_value << 16) & 0xffff0000ffff0000_u64) | ((_value >> 16) & 0x0000ffff0000ffff_u64);
  return (_value << 32) | (_value >> 32);
}

inline constexpr Sint64 swap_s64(Sint64 _value) {
  _value = ((_value << 8) & 0xff00ff00ff00ff00_u64) | ((_value >> 8) & 0x00ff00ff00ff00ff_u64);
  _value = ((_value << 16) & 0xffff0000ffff0000_u64) | ((_value >> 16) & 0x0000ffff0000ffff_u64);
  return (_value << 32) | ((_value >> 32) & 0xffffffff_u64);
}

} // namespace Utility

#endif // RX_CORE_UTILITY_BYTE_SWAP_H