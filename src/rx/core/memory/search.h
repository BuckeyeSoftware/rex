#ifndef RX_CORE_MEMORY_SEARCH_H
#define RX_CORE_MEMORY_SEARCH_H

#include "rx/core/types.h"

/// \file search.h

namespace Rx::Memory {

/// Search memory for a given byte
/// \param _haystack Haystack to search.
/// \param _haystack_size Size of the haystack in bytes.
/// \param _byte The byte value to search.
/// \return Pointer to the first occurance of \p _byte in \p _haystack if found,
/// \c nullptr otherwise.
RX_API Byte* search(const void* _haystack, Size _haystack_size, Byte _byte);

/// Search memory for a needle
/// \param _haystack Haystack to search.
/// \param _haystack_size Size of the haystack in bytes.
/// \param _needle Needle to find.
/// \param _needle_size Size of the needle in bytes.
/// \returns Pointer to the first occurance of \p _needle in \p _haystack if found,
/// \c nullptr otherwise.
RX_API Byte* search(const void* _haystack, Size _haystack_size, const void* _needle, Size _needle_size);

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_SEARCH_H