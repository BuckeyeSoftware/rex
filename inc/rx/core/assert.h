#ifndef RX_CORE_ASSERT_H
#define RX_CORE_ASSERT_H

#include <rx/core/config.h>

namespace rx {

[[noreturn]]
void assert_fail(const char* expression, const char* file,
  const char* function, int line, const char* message, ...);

} // namespace rx

#if defined(__GNUC__)
#define RX_FUNCTION __PRETTY_FUNCTION__
#else // defined(__GNUC__)
#define RX_FUNCTION __func__
#endif // defined(__GNUC__)

#if defined(RX_DEBUG)
#define RX_ASSERT(condition, ...) \
  (static_cast<void>((condition) || (::rx::assert_fail(#condition, __FILE__, RX_FUNCTION, __LINE__, __VA_ARGS__), 0)))
#else // defined(RX_DEBUG)
#define RX_ASSERT(condition, ...) \
  static_cast<void>(0)
#endif // defined(RX_DEBUG)

#if __has_builtin(__builtin_unreachable)
#define RX_UNREACHABLE() \
  __builtin_unreachable()
#else
#define RX_UNREACHABLE() \
  RX_ABORT("reached unreachable code")
#endif

#endif // RX_CORE_ASSERT_H
