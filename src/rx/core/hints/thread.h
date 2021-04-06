#ifndef RX_CORE_HINTS_THREAD_H
#define RX_CORE_HINTS_THREAD_H
#include "rx/core/config.h" // RX_COMPILER_*

/// \file thread.h
/// \brief Thread hints.
#if defined(RX_DOCUMENT)
/// \brief Hint that a variable's access is guarded by a lock.
/// \param _lock A lockable object of type marked with #RX_HINT_LOCKABLE
#define RX_HINT_GUARDED_BY(_lock)

/// \brief Hint that a function acquires a lock.
/// \param ... An expression that yields a lockable.
#define RX_HINT_ACQUIRE(...)

/// \brief Hint that a function releases a lock.
/// \param ... An expression that yields a lockable.
#define RX_HINT_RELEASE(...)

/// Hint that a type is a lockable type.
#define RX_HINT_LOCKABLE
#else
#if defined(RX_COMPILER_CLANG)
#define RX_HINT_THREAD_ATTRIBUTE(...) \
  __attribute__((__VA_ARGS__))
#else
#define RX_HINT_THREAD_ATTRIBUTE(...)
#endif

#define RX_HINT_GUARDED_BY(_lock) \
  RX_HINT_THREAD_ATTRIBUTE(guarded_by(_lock))

#define RX_HINT_ACQUIRE(...) \
  RX_HINT_THREAD_ATTRIBUTE(exclusive_lock_function(__VA_ARGS__))

#define RX_HINT_RELEASE(...) \
  RX_HINT_THREAD_ATTRIBUTE(unlock_function(__VA_ARGS__))

#define RX_HINT_LOCKABLE \
  RX_HINT_THREAD_ATTRIBUTE(lockable)
#endif

#endif // RX_CORE_HINTS_THREAD_H
