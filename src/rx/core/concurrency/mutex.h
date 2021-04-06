#ifndef RX_CORE_CONCURRENCY_MUTEX_H
#define RX_CORE_CONCURRENCY_MUTEX_H
#include "rx/core/types.h" // Byte
#include "rx/core/hints/thread.h"
#include "rx/core/markers.h"

/// \file mutex.h

namespace Rx::Concurrency {

/// \brief Basic mutual exclusion facility.
///
/// A synchronization primitive that can be used to protect shared data from
/// being simultaneously accessed by multiple threads.
///
/// Mutex offers exclusive, non-recursive ownership semantics:
/// * A calling thread _owns_ a mutex from the time that it successfully calls
///   lock() until it calls unlock().
/// * When a thread owns a mutex, all other threads will block (for calls to
///   lock()) if they attempt to claim ownerhip of the mutex.
/// * A calling thread must not own the mutex prior to calling lock().
///
/// \warning It's a bug for a Mutex to be destroyed while still owned by any
/// threads, or a thread terminates while owning a Mutex.
struct RX_API RX_HINT_LOCKABLE Mutex {
  /// @{ Markers
  RX_MARK_NO_COPY(Mutex);
  RX_MARK_NO_MOVE(Mutex);
  /// @}

  /// Constructs the mutex.
  Mutex();

  /// Destroys the mutex.
  ~Mutex();

  /// Locks the mutex, blocks if the mutex is not available.
  ///
  /// Locks the mutex. If another thread has already locked the mutex, a call
  /// to lock() will block execution until the lock is acquired.
  ///
  /// \note Not usually called directly. ScopeLock is encouraged to manage
  /// exclusive locking.
  void lock() RX_HINT_ACQUIRE();

  /// Unlocks the mutex.
  ///
  /// \note Not usually called directly. ScopeLock is encouraged to manage
  /// exclusive locking.
  void unlock() RX_HINT_RELEASE();

private:
  friend struct ConditionVariable;

  // Fixed-capacity storage for any OS mutex Type, adjust if necessary.
  alignas(16) Byte m_mutex[64];
};

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_MUTEX_H
