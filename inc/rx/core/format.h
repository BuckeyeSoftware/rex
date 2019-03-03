#ifndef RX_CORE_FORMAT_H
#define RX_CORE_FORMAT_H

#include <float.h> // {DBL,FLT}_MAX_10_EXP
#include <limits.h> // CHAR_BIT

#include <rx/core/types.h> // rx_size

namespace rx {

// format_size holds maximum size needed to format a value of that type
template<typename T>
struct format_size;

template<>
struct format_size<rx_f32> {
  static constexpr const rx_size size{3+FLT_MANT_DIG-FLT_MIN_EXP};
};

template<>
struct format_size<rx_f64> {
  static constexpr const rx_size size{3+DBL_MANT_DIG-DBL_MIN_EXP};
};

template<>
struct format_size<rx_s32> {
  static constexpr const rx_size size{3+(CHAR_BIT*sizeof(rx_s32)/3)};
};

template<>
struct format_size<rx_s64> {
  static constexpr const rx_size size{3+(CHAR_BIT*sizeof(rx_s32)/3)};
};

template<typename T>
struct format {
  T operator()(const T& value) const {
    return value;
  }
};

template<rx_size E>
struct format<char[E]> {
  const char* operator()(const char (&data)[E]) const {
    return data;
  }
};

} // namespace rx

#endif // RX_CORE_FORMAT_H
