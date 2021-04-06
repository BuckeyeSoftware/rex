#ifndef RX_CORE_HINTS_EMPTY_BASES_H
#define RX_CORE_HINTS_EMPTY_BASES_H
#include "rx/core/config.h" // RX_PLATFORM_WINDOWS

/// \file empty_bases.h
/// \brief Hint base may be empty.
#if defined(RX_DOCUMENT)
/// \brief Used to enable empty base class optimizations.
///
/// Empty base class optimization is required for standard-layout types in order
/// to maintain the requirement that the pointer to a standard-layout object,
/// converted using \c reinterpret_cast, points to its initial member.
///
/// However, on some platforms such as Windows, respecting this newer language
/// requirement would break older, existing code. The following hint provides
/// a mechanism to explicitly opt-in to the newer behavior.
///
/// Example of use:
/// \code{.cpp}
/// struct RX_HINT_EMPTY_BASES Derived : Base {
///   // ...
/// }
/// \endcode
#define RX_HINT_EMPTY_BASES
#else
#if defined(RX_PLATFORM_WINDOWS) && !defined(__MINGW32__)
#define RX_HINT_EMPTY_BASES __declspec(empty_bases)
#else
#define RX_HINT_EMPTY_BASES
#endif
#endif

#endif // RX_CORE_HINTS_EMPTY_BASES_H
