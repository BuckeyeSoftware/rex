#ifndef RX_CORE_HASH_MIX_INT_H
#define RX_CORE_HASH_MIX_INT_H
#include "rx/core/types.h"
#include "rx/core/concepts/integral.h"
#include "rx/core/traits/is_same.h"

#include "rx/core/hints/unreachable.h"

namespace Rx::Hash {

inline constexpr Size mix_bool(bool _value) {
  return _value ? 1231 : 1237;
}

inline constexpr Size mix_uint8(Uint8 _value) {
  Size hash = _value * 251;
  hash += ~(_value << 3);
  hash ^= (_value >> 1);
  hash += ~(_value << 7);
  hash ^= (_value >> 6);
  hash += (_value << 2);
  return hash;
}

inline constexpr Size mix_uint16(Uint16 _value) {
  const Size z = (_value << 8) | (_value >> 8);
  Size hash = z;
  hash += ~(z << 5);
  hash ^= (z >> 2);
  hash += ~(z << 13);
  hash ^= (z >> 10);
  hash += ~(z << 4);
  hash = (hash << 10) | (hash >> 10);
  return hash;
}

inline constexpr Size mix_uint32(Uint32 _value) {
  _value = (_value ^ 61) ^ (_value >> 16);
  _value = _value + (_value << 3);
  _value = _value ^ (_value >> 4);
  _value = _value * 0x27D4EB2D;
  _value = _value ^ (_value >> 15);
  return static_cast<Size>(_value);
}

inline constexpr Size mix_uint64(Uint64 _value) {
  _value = (~_value) + (_value << 21);
  _value = _value ^ (_value >> 24);
  _value = (_value + (_value << 3)) + (_value << 8);
  _value = _value ^ (_value >> 14);
  _value = (_value + (_value << 2)) + (_value << 4);
  _value = _value ^ (_value << 28);
  _value = _value + (_value << 31);
  return static_cast<Size>(_value);
}

// Signed variants of just cast to unsigned.
inline constexpr Size mix_sint8(Sint8 _value) {
  return mix_uint8(static_cast<Uint8>(_value));
}

inline constexpr Size mix_sint16(Sint16 _value) {
  return mix_uint8(static_cast<Uint16>(_value));
}

inline constexpr Size mix_sint32(Sint32 _value) {
  return mix_uint8(static_cast<Uint32>(_value));
}

inline constexpr Size mix_sint64(Sint64 _value) {
  return mix_uint8(static_cast<Uint64>(_value));
}

template<Concepts::Integral T>
inline constexpr Size mix_int(T _value) {
  if constexpr (Traits::IS_SAME<T, bool>) {
    return mix_bool(_value);
  } else if constexpr (Traits::IS_SAME<T, Uint8>) {
    return mix_uint8(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Sint8>) {
    return mix_sint8(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Uint16>) {
    return mix_uint16(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Sint16>) {
    return mix_sint16(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Uint32>) {
    return mix_uint32(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Sint32>) {
    return mix_sint32(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Uint64>) {
    return mix_uint64(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Sint64>) {
    return mix_sint64(static_cast<T>(_value));
  } else if constexpr (Traits::IS_SAME<T, Size>) {
    // Size may not have the same type as any of the previously listed
    // sized integer types.
    //
    // Emscripten for instance will have:
    //   Uint32 = unsigned int
    //   Uint64 = unsigned long long
    //   Size   = unsigned long
    if constexpr (sizeof(Size) == 4) {
      return mix_uint32(static_cast<Uint32>(_value));
    } else {
      return mix_uint64(static_cast<Uint64>(_value));
    }
  }
  RX_HINT_UNREACHABLE();
}

} // namespace Rx::Hash

#endif // RX_CORE_HASH_MIX_INT_H
