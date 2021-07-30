#ifndef RX_CORE_UTILITY_BIT_H
#define RX_CORE_UTILITY_BIT_H
#include "rx/core/types.h"

namespace Rx {

/// Search for least significant set bit.
/// \param _bits The bits to search in.
/// \return The index of the set bit.
template<typename T>
Size bit_search_lsb(T _bits);

/// Calculate number of 1 bits.
/// \param _bits The bits to count set bits of.
/// \return The number of set bits.
template<typename T>
Size bit_pop_count(T _bits);

// use compiler intrinsics if available
#if defined(RX_COMPILER_CLANG) || defined(RX_COMPILER_GCC)
template<>
inline Size bit_search_lsb(Uint32 _bits) {
  return _bits ? __builtin_ctzl(_bits) : 32;
}

template<>
inline Size bit_search_lsb(Uint64 _bits) {
  return _bits ? __builtin_ctzll(_bits) : 64;
}

template<>
inline Size bit_pop_count(Uint32 _bits) {
  return __builtin_popcountl(_bits);
}

template<>
inline Size bit_pop_count(Uint64 _bits) {
  return __builtin_popcountll(_bits);
}
#else
// portable implementations that optimize quite well
template<>
inline Size bit_search_lsb(Uint32 _bits) {
  static constexpr const Byte TABLE[] = {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 31, 27, 13, 23,
    21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return _bits ? TABLE[Uint32{_bits * 0x077cb531} >> 27] : 32;
}

template<>
inline Size bit_search_lsb(Uint64 _bits) {
  static constexpr const Byte TABLE[] = {
    0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28, 62, 5, 39, 46, 44,
    42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11, 63, 52, 6, 26, 37, 40, 33, 47,
    61, 45, 43, 21, 23, 58, 17, 10, 51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19,
    15, 30, 14, 13, 12
  };
  return _bits ? TABLE[Uint64{(_bits & -_bits) * 0x022fdd63cc95386d} >> 58] : 64;
}

template<>
inline Size bit_pop_count(Uint32 _bits) {
  // hamming weight to count set bits; 17 arithmetic ops on x86_64
  static constexpr const Uint32 M1 = 0x55555555; // binary: 1 zeros, 1 ones repeating: 01010101...
  static constexpr const Uint32 M2 = 0x33333333; // binary: 2 zeros, 2 ones repeating: 00110011...
  static constexpr const Uint32 M4 = 0x0f0f0f0f; // binary: 4 zeros, 4 ones repeating: 00001111...
  static constexpr const Uint32 H0 = 0x01010101; // sum of 256 to the power of 0,1,2,3...

  _bits -= ((_bits >> 1) & M1);
  _bits = (_bits & M2) + ((_bits >> 2) & M2);
  _bits = (_bits + (_bits >> 4)) & M4;

  // return left 8 bits
  return (_bits * H0) >> 24;
}

template<>
inline Size bit_pop_count(Uint64 _bits) {
  // hamming weight to count set bits; 17 arithmetic ops on x86_64
  static constexpr const Uint64 M1 = 0x5555555555555555; // binary: 1 zeros, 1 ones repeating: 01010101...
  static constexpr const Uint64 M2 = 0x3333333333333333; // binary: 2 zeros, 2 ones repeating: 00110011...
  static constexpr const Uint64 M4 = 0x0f0f0f0f0f0f0f0f; // binary: 4 zeros, 4 ones repeating: 00001111...
  static constexpr const Uint64 H0 = 0x0101010101010101; // sum of 256 to the power of 0,1,2,3...

  _bits -= ((_bits >> 1) & M1);
  _bits = (_bits & M2) + ((_bits >> 2) & M2);
  _bits = (_bits + (_bits >> 4)) & M4;

  // return left 8 bits
  return (_bits * H0) >> 56;
}
#endif

/// Find the next set bit.
/// \param _bits The bits to search in.
/// \param _bit The current bit index.
/// \return The index of the next set bit.
template<typename T>
Size bit_next(T _bits, Size _bit) {
  return bit_search_lsb<T>(_bits & ~((T{1} << _bit) - 1));
}

} // namespace Rx

#endif // RX_CORE_UTILITY_BIT_H
