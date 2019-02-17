#ifndef RX_CORE_HASH_H
#define RX_CORE_HASH_H

#include <rx/core/types.h>

namespace rx {

template<typename T>
struct hash;

template<>
struct hash<bool> {
  rx_size operator()(bool value) const {
    return value ? 1231 : 1237;
  }
};

template<>
struct hash<rx_u32> {
  rx_size operator()(rx_u32 value) const {
    value = (value ^ 61) ^ (value >> 16);
    value = value + (value << 3);
    value = value ^ (value >> 4);
    value = value * 0x27D4EB2D;
    value = value ^ (value >> 15);
    return static_cast<rx_size>(value);
  }
};

template<>
struct hash<rx_s32> {
  rx_size operator()(rx_s32 value) const {
    return hash<rx_u32>{}(static_cast<rx_u32>(value));
  }
};

template<>
struct hash<rx_u64> {
  rx_size operator()(rx_u64 value) const {
    value = (~value) + (value << 21);
    value = value ^ (value >> 24);
    value = (value + (value << 3)) + (value << 8);
    value = value ^ (value >> 14);
    value = (value + (value << 2)) + (value << 4);
    value = value ^ (value << 28);
    value = value + (value << 31);
    return static_cast<rx_size>(value);
  }
};

template<>
struct hash<rx_s64> {
  rx_size operator()(rx_s64 value) const {
    return hash<rx_u64>{}(static_cast<rx_u64>(value));
  }
};

template<typename T>
struct hash<T*> {
  rx_size operator()(T* value) const {
    if constexpr (sizeof value == 8) {
      return hash<rx_u64>{}(reinterpret_cast<rx_u64>(value));
    } else {
      return hash<rx_u32>{}(reinterpret_cast<rx_u32>(value));
    }
  }
};

} // namespace rx

#endif // RX_CORE_HASH_H
