#ifndef RX_FOUNDATION_TYPES_H
#define RX_FOUNDATION_TYPES_H

using rx_size = decltype(sizeof 0);
using rx_byte = unsigned char;
using rx_s8 = signed char;
using rx_u8 = unsigned char;
using rx_s16 = signed short;
using rx_u16 = unsigned short;
using rx_s32 = signed int;
using rx_u32 = unsigned int;
using rx_s64 = signed long long;
using rx_u64 = unsigned long long;
using rx_f32 = float;
using rx_f64 = double;
using rx_ptrdiff = long;

constexpr rx_size operator"" _z(unsigned long long value) {
  return static_cast<rx_size>(value);
}

#endif // RX_FOUNDATION_TYPES_H
