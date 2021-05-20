#ifndef RX_CORE_CONCURRENCY_SPIN_LOCK_H
#define RX_CORE_CONCURRENCY_SPIN_LOCK_H
#include "rx/core/concurrency/atomic.h" // AtomicFlag
#include "rx/core/hints/thread.h" // RX_HINT_{LOCKABLE, ACQUIRE, RELEASE}

/// \file spin_lock.h

namespace Rx::Concurrency {

/// \brief Low-level mutual exclusion.
///
/// A syncronization primitive that can be used to protect shared data from
/// being accessed by multiple threads.
///
/// SpinLock offers exclusive, non-recursive ownership semantics.
/// * A calling thread owns a SpinLock from the time that it calls lock() until
///   it calls unlock().
/// * When a thread owns a SpinLock, all other threads will block (for calls to
///   lock()) if they attempt to claim ownership of the SpinLock.
///
/// \warning Use of spin locks are not applicable as a general locking solution,
/// as they're, by definition, prone to priority inversion and unbounded spin
/// times.
///
/// \warning If a thread creates a deadlock situation employing spin locks,
/// those threads will spin forever consuming CPU time.
struct RX_API RX_HINT_LOCKABLE SpinLock {
  /// Construct a SpinLock.
  constexpr SpinLock() = default;

  /// Destruct a SpinLock.
  ~SpinLock() = default;

  /// Locks the SpinLock, blocks if the SpinLock is not available.
  ///
  /// Locks the SpinLock. If another thread has already locked the SpinLock, a
  /// call to lock() will block execution until the lock is acquired.
  void lock() RX_HINT_ACQUIRE();

  /// Unlocks the SpinLock.
  void unlock() RX_HINT_RELEASE();

private:
  AtomicFlag m_lock { false };
};

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_SPIN_LOCK_H
