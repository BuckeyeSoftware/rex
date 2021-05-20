#ifndef RX_CORE_CONCURRENCY_WORD_LOCK_H
#define RX_CORE_CONCURRENCY_WORD_LOCK_H
#include "rx/core/concurrency/atomic.h" // Atomic<T>
#include "rx/core/hints/thread.h" // RX_HINT_{LOCKABLE, ACQUIRE, RELEASE}

namespace Rx::Concurrency {

/// \brief Mid-level mutual exclusion
///
/// A syncronization primitive that can be used to protect shared data from
/// being accessed by multiple threads.
///
/// WordLock offers exclusive, non-recursive ownership semantics.
/// * A calling thread owns a WordLock from the time that it calls lock() until
///   it calls unlock().
/// * When a thread owns a WordLock, all other threads will block (for calls to
///   lock()) if they attempt to claim ownership of the WordLock.
///
/// A WordLock is an adaptive mutex that uses the same storage as a pointer.
/// It has an extremely fast path that is similar to SpinLock, and a slow path
/// that is similar to Mutex. In most cases when you do not want a full blown
/// Mutex and would reach for a SpinLock, you should use a WordLock instead.
///
/// This is based on the WordLock implementation in WebKit's WTF
/// for more information read:
///  https://github.com/WebKit/WebKit/blob/main/Source/WTF/wtf/WordLock.h
///  https://github.com/WebKit/WebKit/blob/main/Source/WTF/wtf/WordLock.cpp
///  https://webkit.org/blog/6161/locking-in-webkit/
struct RX_API RX_HINT_LOCKABLE WordLock {
  /// Construct a WordLock.
  constexpr WordLock() = default;

  /// Destruct a WordLock.
  ~WordLock() = default;

  /// Locks the WordLock, blocks if the WordLock is not available.
  ///
  /// Locks the WordLock. If another thread has already locked the WordLock, a
  /// call to lock() will block execution until the lock is acquired.
  void lock() RX_HINT_ACQUIRE();

  /// Unlocks the WordLock.
  void unlock() RX_HINT_RELEASE();

private:
  void lock_slow();
  void unlock_slow();

  Atomic<UintPtr> m_word { 0 };
};

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_WORD_LOCK_H