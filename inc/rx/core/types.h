#ifndef RX_FOUNDATION_TYPES_H
#define RX_FOUNDATION_TYPES_H

namespace detail {
  template<bool B, typename T, typename F>
  struct match_type {
    using type = T;
  };

  template<typename T, typename F>
  struct match_type<false, T, F> {
    using type = F;
  };

  template<typename T1, typename T2>
  using qword = typename match_type<sizeof(T1) == 8, T1, T2>::type;
} // namespace detail

using rx_size = decltype(sizeof 0);
using rx_byte = unsigned char;
using rx_s8 = signed char;
using rx_u8 = unsigned char;
using rx_s16 = signed short;
using rx_u16 = unsigned short;
using rx_s32 = signed int;
using rx_u32 = unsigned int;
using rx_s64 = detail::qword<signed long, signed long long>;
using rx_u64 = detail::qword<unsigned long, unsigned long long>;
using rx_f32 = float;
using rx_f64 = double;
using rx_ptrdiff = long;
using rx_uintptr = rx_size;

constexpr rx_size operator"" _z(unsigned long long value) {
  return static_cast<rx_size>(value);
}

#endif // RX_FOUNDATION_TYPES_H
