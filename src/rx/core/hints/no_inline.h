#ifndef RX_CORE_HINTS_NO_INLINE_H
#define RX_CORE_HINTS_NO_INLINE_H
#include "rx/core/config.h" // RX_COMPILER_{GCC,CLANG,MSVC}

/// \file no_inline.h
/// \brief Prevent inlining
#if defined(RX_DOCUMENT)
/// Hints to the compiler that a given function should not be inlined.
/// \warning Use of this hint disables optimizations.
#define RX_HINT_NO_INLINE
#else
#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HINT_NO_INLINE __attribute__((noinline))
#elif defined(RX_COMPILER_MSVC)
#define RX_HINT_NO_INLINE
#else
#define RX_HINT_NO_INLINE
#endif
#endif

#endif // RX_CORE_HINTS_NO_INLINE_H
