#ifndef RX_CORE_HINTS_FORCE_INLINE_H
#define RX_CORE_HINTS_FORCE_INLINE_H
#include "rx/core/config.h" // RX_COMPILER_{GCC,CLANG,MSVC}

/// \file force_inline.h
/// \brief Force inlining.
#if defined(RX_DOCUMENT)
/// \brief Used to force inlining.
///
/// Hint to force the alignment of a specific function, even while compiler
/// optimizations are disabled.
///
/// This should only be used on very small, trivial functions, such as setters
/// and getters that don't call other inlined functions.
#define RX_HINT_FORCE_INLINE
#else
#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HINT_FORCE_INLINE inline __attribute__((always_inline))
#define RX_HINT_FORCE_INLINE_LAMBDA __attribute__((always_inline))
#elif defined(RX_COMPILER_MSVC)
#define RX_HINT_FORCE_INLINE __forceinline
#define RX_HINT_FORCE_INLINE_LAMBDA [[msvc::forceinline]]
#else
#define RX_HINT_FORCE_INLINE inline
#define RX_HINT_FORCE_INLINE_LAMBDA
#endif
#endif

#endif // RX_CORE_HINTS_FORCE_INLINE_H
