#ifndef RX_FOUNDATION_TYPES_H
#define RX_FOUNDATION_TYPES_H
#include "rx/core/config.h"
#include "rx/core/traits/conditional.h"

/// \file types.h

/// Root namespace.
namespace Rx {

/// An alias for \c size_t
using Size = decltype(sizeof 0);
/// An alias for \c bool
using Bool = bool;
/// A type which represents a byte.
using Byte = unsigned char;
/// Signed 8-bit integer.
using Sint8 = signed char;
/// Unsigned 8-bit integer.
using Uint8 = unsigned char;
/// Signed 16-bit integer.
using Sint16 = signed short;
/// Unsigned 16-bit integer.
using Uint16 = unsigned short;
/// Signed 32-bit integer.
using Sint32 = signed int;
/// Unsigned 32-bit integer.
using Uint32 = unsigned int;
/// Signed 64-bit integer.
using Sint64 = Rx::Traits::Conditional<sizeof(signed long) == 8, signed long, signed long long>;
/// Unsigned 64-bit integer.
using Uint64 = Rx::Traits::Conditional<sizeof(unsigned long) == 8, unsigned long, unsigned long long>;
/// 32-bit float.
using Float32 = float;
/// 64-bit float.
using Float64 = double;
/// The different of two pointers.
using PtrDiff = decltype((Byte*)0 - (Byte*)0);
/// Unsigned integer large enough to hold a pointer.
using UintPtr = Size;
/// Alias for \c nullptr_t.
using NullPointer = decltype(nullptr);
/// The type 32-bit floating point arithmetic is evaluated in on the hardware.
using Float32Eval = Float32;
/// The type 64-bit floating point arithmetic is evaluated in on the hardware.
using Float64Eval = Float64;

/// Convenience user-defined literal for producing Size literals.
constexpr Size operator"" _z(unsigned long long _value) {
  return static_cast<Size>(_value);
}

/// Convenience user-defined literal for producing Uint8 literals.
constexpr Uint8 operator"" _u8(unsigned long long _value) {
  return static_cast<Uint8>(_value);
}

/// Convenience user-defined literal for producing Uint16 literals.
constexpr Uint16 operator"" _u16(unsigned long long _value) {
  return static_cast<Uint16>(_value);
}

/// Convenience user-defined literal for producing Uint32 literals.
constexpr Uint32 operator"" _u32(unsigned long long _value) {
  return static_cast<Uint32>(_value);
}

/// Convenience user-defined literal for producing Uint64 literals.
constexpr Uint64 operator"" _u64(unsigned long long _value) {
  return static_cast<Uint64>(_value);
}

/// Convenience user-defined literal for KiB to bytes.
constexpr auto operator""_KiB(unsigned long long _value) {
  return _value * 1024ul;
}

/// Convienence user-defined literal for MiB to bytes.
constexpr auto operator""_MiB(unsigned long long _value) {
  return _value * 1024_KiB;
}

/// Convienence user-defined literal for GiB to bytes.
constexpr auto operator""_GiB(unsigned long long _value) {
  return _value * 1024_MiB;
}

} // namespace Rx

#endif // RX_FOUNDATION_TYPES_H
