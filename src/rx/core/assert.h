#ifndef RX_CORE_ASSERT_H
#define RX_CORE_ASSERT_H
#include "rx/core/config.h" // __has_builtin, RX_COMPILER_{GCC, CLANG}
#include "rx/core/abort.h" // rx::abort

namespace rx {

[[noreturn]]
void assert_fail(const char* _expression, const char* _file,
  const char* _function, int _line, const char* _message, ...);

} // namespace rx

#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_FUNCTION __PRETTY_FUNCTION__
#else // defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_FUNCTION __func__
#endif // defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)

#if defined(RX_DEBUG)
#define RX_ASSERT(condition, ...) \
  (static_cast<void>((condition) || (::rx::assert_fail(#condition, __FILE__, RX_FUNCTION, __LINE__, __VA_ARGS__), 0)))
#else // defined(RX_DEBUG)
#define RX_ASSERT(condition, ...) \
  static_cast<void>(0)
#endif // defined(RX_DEBUG)

#endif // RX_CORE_ASSERT_H
