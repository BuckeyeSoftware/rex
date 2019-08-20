#ifndef RX_CORE_HINT_H
#define RX_CORE_HINT_H
#include "rx/core/config.h"

#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HINT_LIKELY(x)    __builtin_expect(!!(x), 1)
#define RX_HINT_UNLIKELY(x)  __builtin_expect(!!(x), 0)
#else
#define RX_HINT_LIKELY(x)    (x)
#define RX_HINT_UNLIKELY(x)  (x)
#endif // defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)

#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HINT_UNREACHABLE() \
  __builtin_unreachable()
#else
#define RX_HINT_UNREACHABLE()
#endif

#endif // RX_CORE_HINT_H