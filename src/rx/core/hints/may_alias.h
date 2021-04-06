#ifndef RX_CORE_HINTS_MAY_ALIAS_H
#define RX_CORE_HINTS_MAY_ALIAS_H
#include "rx/core/config.h"

/// \file may_alias.h
/// \brief Hint to indicate aliasing.
#if defined(RX_DOCUMENT)
/// Hint that type may alias.
///
/// On some compilers which implement strict-aliasing optimizations, such as gcc
/// and clang, producing an aliasing violation leads to undefined behavior and
/// a license for such compilers to miscompile code.
///
/// The following hint designates for accesses to objects with this type
/// attribute, that such accesses are not subjected to type-based alias
/// analysis, but are instead assumed to be able to alias any other type of
/// objects.
///
/// \warning Use of this hint will disable optimizations.
#define RX_HINT_MAY_ALIAS
#else
#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HINT_MAY_ALIAS __attribute__((__may_alias__))
#else
#define RX_HINT_MAY_ALIAS
#endif
#endif

#endif // RX_CORE_HINTS_MAY_ALIAS_H
