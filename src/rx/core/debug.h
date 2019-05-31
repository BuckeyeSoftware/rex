#ifndef RX_CORE_DEBUG_H
#define RX_CORE_DEBUG_H

#include "rx/core/assert.h" // RX_FUNCTION

namespace rx {

void debug_message(const char* file, const char* function, int line,
  const char* message, ...);

} // namespace rx

#if defined(RX_DEBUG)
#define RX_MESSAGE(...) \
  ::rx::debug_message(__FILE__, RX_FUNCTION, __LINE__, __VA_ARGS__)
#else // defined(RX_DEBUG)
#define RX_MESSAGE(...) \
  static_cast<void>(0)
#endif // defined(RX_DEBUG)

#endif // RX_CORE_DEBUG_H
