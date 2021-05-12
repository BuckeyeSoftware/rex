#ifndef RX_CORE_MEMORY_SEARCH_H
#define RX_CORE_MEMORY_SEARCH_H

#include "rx/core/types.h"

/// \file search.h

namespace Rx::Memory {

void* search(const void* _haystack, Size _haystack_size, const void* _needle, Size _needle_size);

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_SEARCH_H