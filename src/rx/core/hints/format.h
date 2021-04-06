#ifndef RX_CORE_HINTS_FORMAT_H
#define RX_CORE_HINTS_FORMAT_H
#include "rx/core/config.h" // RX_COMPILER_{GCC, CLANG}

/// \file format.h
/// \brief Hint functions take a format string.
///
/// This hint enables additional type-checking in compilers that support it on
/// the given format string.
///
/// The hint allows you to identify your own functions which take format strings
/// as arguments so that the compiler can check the calls to these functions
/// for errors.
#if defined(RX_DOCUMENT)
/// \brief Mark a function as a formatting one.
/// \param _format_index Specifies which argument is the format string argument
/// (starting from 1).
/// \param _list_index The number of the first argument to check against the
/// format string.
///
/// \note For functions where the arguments are not available to be checked
/// (for example, \c vprintf ), specify zero for \p _list_index. In this case
/// the compiler only checks the format string for consistency.
///
/// \note Member functions of a class have an implicit first argument of
/// \c this, thus, \p _format_index and \p _list_index for such functions should
/// begin from index \p 2.
#define RX_HINT_FORMAT(_format_index, _list_index)
#else
#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HINT_FORMAT(_format_index, _list_index) \
  __attribute__((format(printf, _format_index, _list_index)))
#else
#define RX_HINT_FORMAT(...)
#endif
#endif

#endif // RX_CORE_HINTS_FORMAT_H
