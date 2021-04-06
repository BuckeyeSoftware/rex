#ifndef RX_CORE_CONCURRENCY_RECURSIVE_MUTEX_H
#define RX_CORE_CONCURRENCY_RECURSIVE_MUTEX_H
#include "rx/core/types.h" // Byte
#include "rx/core/hints/thread.h"
#include "rx/core/markers.h"

/// \file recursive_mutex.h

namespace Rx::Concurrency {

/// \brief Basic mutual exclusion facility.
///
/// A synchronization primitive that can be used to protect shared data from
/// being simultaneously accessed by multiple threads.
///
/// RecursiveMutex offers exclusive, non-recursive ownership semantics:
/// * A calling thread _owns_ a `RecursiveMutex` for a period of time that
///   starts when it successfully calls lock(). During this period, the thread
///   may make additional calls to lock(). The period of ownership ends when the
///   thread makes a matching number of calls to unlock().
/// * When a thread owns a RecursiveMutex, all other threads will block (for
///   calls to lock()) if they attempt to claim ownerhip of the RecursiveMutex
///
/// \warning It's a bug for a Mutex to be destroyed while still owned by some
/// thread.
struct RX_API RX_HINT_LOCKABLE RecursiveMutex {
  /// @{ Markers
  RX_MARK_NO_COPY(RecursiveMutex);
  RX_MARK_NO_MOVE(RecursiveMutex);
  /// @}

  /// Constructs the mutex.
  RecursiveMutex();
  /// Destroys the mutex.
  ~RecursiveMutex();

  /// Locks the mutex, blocks if the mutex is not available.
  ///
  /// Locks the mutex. If another thread has already locked the mutex, a call
  /// to `lock` will block execution until the lock is acquired.
  ///
  /// A thread may call `lock` on a `RecursiveMutex` repeatredly. Ownership
  /// will only be released after the thread makes a matching number of calls
  /// to unlock().
  ///
  /// \note Not usually called directly. ScopeLock is encouraged to manage
  /// exclusive locking.
  void lock() RX_HINT_ACQUIRE();

  /// Unlocks the mutex.
  ///
  /// Unlocks the mutex if its level of ownership is `1` (there was exactly
  /// one more call to lock() than there were calls to unlock() made by this
  /// thread), reduces the ownership by 1 otherwise.
  ///
  /// \warning The mutex must be locked by the current thread of execution.
  ///
  /// \note Not usually called directly. ScopeLock is encouraged to manage
  /// exclusive locking.
  void unlock() RX_HINT_RELEASE();

private:
  friend struct ConditionVariable;

  // Fixed-capacity storage for any OS mutex type, adjust if necessary.
  alignas(16) Byte m_mutex[64];
};

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_RECURSIVE_MUTEX_H
