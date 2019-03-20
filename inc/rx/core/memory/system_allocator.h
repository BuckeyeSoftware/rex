#ifndef RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#define RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H

#include <rx/core/memory/stats_allocator.h>
#include <rx/core/statics.h>

namespace rx::memory {

extern static_global<stats_allocator> g_system_allocator;

} // namespace rx::memory

#endif // RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
