#ifndef RX_CORE_ASSERT_H
#define RX_CORE_ASSERT_H
#include "rx/core/source_location.h"

namespace rx {

[[noreturn]]
void assert_fail(const char* _expression,
  const source_location& _source_location, const char* _message, ...);

} // namespace rx

#if defined(RX_DEBUG)
#define RX_ASSERT(_condition, ...) \
  (static_cast<void>((_condition) \
    || (::rx::assert_fail(#_condition, RX_SOURCE_LOCATION, __VA_ARGS__), 0)))
#else // defined(RX_DEBUG)
#define RX_ASSERT(_condition, ...) \
  static_cast<void>((_condition) || 0)
#endif // defined(RX_DEBUG)

#endif // RX_CORE_ASSERT_H
