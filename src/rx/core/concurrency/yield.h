#ifndef RX_CORE_CONCURRENCY_YIELD_H
#define RX_CORE_CONCURRENCY_YIELD_H
#include "rx/core/config.h" // RX_API

/// \file yield.h

namespace Rx::Concurrency {

/// \brief Yield the processor.
///
/// Causes the calling thread to relinquish the CPU.
///
/// The yield of execution is in effect for up to one thread-scheduling time
/// slice on the processor of the calling thread.
RX_API void yield();

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_YIELD_H
