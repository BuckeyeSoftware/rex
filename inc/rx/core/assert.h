#ifndef RX_CORE_ASSERT_H
#define RX_CORE_ASSERT_H

namespace rx {

[[noreturn]]
void assert_fail(const char* expression, const char* message,
  const char* file, const char* function, int line);

} // namespace rx

#if defined(__GNUC__)
#define RX_FUNCTION __PRETTY_FUNCTION__
#else // defined(__GNUC__)
#define RX_FUNCTION __func__
#endif // defined(__GNUC__)

#if defined(RX_DEBUG)
#define RX_ASSERT(condition, message) \
  (static_cast<void>((condition) || (::rx::assert_fail(#condition, (message), __FILE__, RX_FUNCTION, __LINE__), 0)))
#else // defined(RX_DEBUG)
#define RX_ASSERT(condition, message) \
  static_cast<void>(0)
#endif // defined(RX_DEBUG)

#endif // RX_CORE_ASSERT_H
