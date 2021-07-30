#ifndef RX_CORE_FORMAT_H
#define RX_CORE_FORMAT_H
#include <float.h> // {DBL,FLT}_MAX_10_EXP
#include <stdarg.h> // va_list, va_{start, copy, end}

#include "rx/core/span.h"
#include "rx/core/utility/forward.h"
#include "rx/core/traits/remove_cvref.h"
#include "rx/core/hints/format.h"
#include "rx/core/concepts/integral.h"

namespace Rx {

// FormatSize holds maximum size needed to format a value of that type
template<typename T>
struct FormatSize;

template<>
struct FormatSize<Float32> {
  static constexpr const Size SIZE = 3 + FLT_MANT_DIG - FLT_MIN_EXP;
};

template<>
struct FormatSize<Float64> {
  static constexpr const Size SIZE = 3 + DBL_MANT_DIG - DBL_MIN_EXP;
};

// General rule is n binary digits require ceil(n*ln(2)/ln(10)) = ceil(n * 0.301)
// bytes to format, not including null-terminator.
//
// Okay to waste some space here approximating ln(2)/ln(10). Signed requires
// more for a possible sign character.
template<Concepts::SignedIntegral T>
struct FormatSize<T> {
  static constexpr const Size SIZE = 3 + (8 * sizeof(T) / 3);
};

template<Concepts::UnsignedIntegral T>
struct FormatSize<T> {
  static constexpr const Size SIZE = (8 * sizeof(T) + 5) / 3;
};

// FormatNormalize is used to convert non-trivial |T| into |...| compatible types.
template<typename T>
struct FormatNormalize {
  constexpr T operator()(const T& _value) const {
    return _value;
  }
};

template<Size E>
struct FormatNormalize<char[E]> {
  constexpr const char* operator()(const char (&_data)[E]) const {
    return _data;
  }
};

/// @{
/// Low-level format functions.
RX_API Size format_buffer_va_list(Span<char> buffer_, const char* _format, va_list _list) RX_HINT_FORMAT(2, 0);
RX_API Size format_buffer_va_args(Span<char> buffer_, const char* _format, ...); /*RX_HINT_FORMAT(2, 3);*/
/// @}

/// Format string into a buffer.
/// \param buffer_ The buffer to format into.
/// \param _format The format string.
/// \param _arguments The format arguments.
/// \return The number of bytes successfully formatted into \p buffer_.
template<typename... Ts>
RX_HINT_FORMAT(2, 0) Size format_buffer(Span<char> buffer_, const char* _format, Ts&&... _arguments) {
  return format_buffer_va_args(buffer_, _format,
    FormatNormalize<Traits::RemoveCVRef<Ts>>{}(Utility::forward<Ts>(_arguments))...);
}

} // namespace Rx

#endif // RX_CORE_FORMAT_H
