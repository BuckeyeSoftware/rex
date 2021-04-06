#ifndef RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H
#define RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H
#include "rx/core/types.h"
#include "rx/core/concurrency/scope_lock.h"

/// \file condition_variable.h

namespace Rx::Concurrency {

struct Mutex;
struct RecursiveMutex;

/// \brief Condition variable associated with a lockable.
///
/// Synchronization primitive that can be used to block a thread, or multiple
/// threads at the same time until another thread both modifies a shared
/// variable (the _condition_), and notifies the ConditionVariable.
///
/// A thread that intends to modify the shared variable has to
/// 1. Acquire a Mutex or RecursiveMutex (typically via ScopeLock)
/// 2. Perform the modification while the lock is held.
/// 3. Execute signal() or broadcast() on the ConditionVariable (the lock does
///    not need to be held for notification.)
///
/// Even if the shared variable is atomic, it must be modified under a mutex
/// inorder to correctly publish the modification to the waiting thread.
/// Any thread that intends to wait on a ConditionVariable has to:
/// 1. Acquire a Mutex or RecursiveMutex (typically via ScopeLock)
/// 2. Either
///
///     1. Check the condition, in case it was already updated and notified.
///     2. Execute wait(). The wait operations atomically release the mutex
///        and suspend the execution of the thread.
///     3. When the condition variable is notified, or a spurious wakeup occurs,
///        the thread is awakened, and the mutex is atomically reacquired. The
///        thread should then check the condition and resume waiting if the wake
///        up was supurious.
///
///    Or
///
///     1. Use the predicated overload of wait(), which takes care of all the
///        steps above.
///
/// Condition variables permit concurrenct invocation of the \ref wait, \ref
/// signal, and \ref broadcast member functions.
struct RX_API ConditionVariable {
  ConditionVariable();
  ~ConditionVariable();

  /// @{
  /// \brief Blocks the current thread until the condition variable is woken up.
  ///
  /// Atomically unlocks \p _mutex, blocks the current executing thread. The
  /// thread will be unblocked when broadcast() or signal() is executed. It may
  /// also be unblocked spuriously. When unblocked, regardless of the reason,
  /// \p _mutex is reacquired and wait() exits.
  ///
  /// \param _mutex The mutex, which must be locked by the current thread.
  void wait(Mutex& _mutex);
  void wait(RecursiveMutex& _mutex);
  /// @}

  /// @{
  /// \brief Blocks the current thread until the condition variable is woken up.
  ///
  /// Atomically unlocks the mutex owned by \p _scope_lock. The thread will be
  /// unblocked when broadcast() or signal() is executed. It may also be
  /// unblocked spuriously. When unblocked, regardless of the reason, the mutex
  /// owned by \p _scope_lock is reacquired and wait() exits.
  ///
  /// \param _scope_lock The scoped lock.
  void wait(ScopeLock<Mutex>& _scope_lock);
  void wait(ScopeLock<RecursiveMutex>& _scope_lock);
  /// @}

  /// @{
  /// \brief Blocks the current thread until the condition variable is woken up.
  ///
  /// Equivalent to:
  /// \code{.cpp}
  /// while (!_predicate()) {
  ///   wait(_mutex);
  /// }
  /// \endcode
  ///
  /// This overload may be used to ignore spurious awakenings while waiting for
  /// a specific condition to become true.
  ///
  /// \param _mutex The mutex, which must be locked by the current thread.
  /// \param _predicate Predicate which returns `false` if the waiting should
  /// be continued. The signature of the predicate function should be equivalent
  /// to the following:
  ///   \code{.cpp}
  ///   bool predicate();
  ///   \endcode
  ///
  /// \note The mutex must be acquired before entering this method, and it
  /// is reacquired after `wait(_mutex)` exits, which means that the lock
  /// can be used to guard access to `_predicate()`.
  template<typename P>
  void wait(Mutex& _mutex, P&& _predicate);
  template<typename P>
  void wait(RecursiveMutex& _mutex, P&& _predicate);
  /// @}

  /// @{
  /// \brief Blocks the current thread until the condition variable is woken up.
  ///
  /// Equivalent to:
  ///   \code{.cpp}
  ///   while (!_predicate()) {
  ///     wait(_scope_lock);
  ///   }
  ///   \endcode
  ///
  /// This overload may be used to ignore spurious awakenings while waiting for
  /// a specific condition to become true.
  ///
  /// \param _scope_lock The scope lock.
  /// \param _predicate Predicate which returns `false` if the waiting should
  /// be continued. The signature of the predicate function should be equivalent
  /// to the following:
  ///   \code{.cpp}
  ///   bool predicate();
  ///   \endcode
  template<typename P>
  void wait(ScopeLock<Mutex>& _scope_lock, P&& _predicate);
  template<typename P>
  void wait(ScopeLock<RecursiveMutex>& _scope_lock, P&& _predicate);
  /// @}

  /// @{
  /// \brief Notifies one waiting thread.
  /// If any threads are waiting on this, calling signal() unblocks one of the
  /// waiting threads.
  void signal();
  /// \brief Notifies all waiting threads.
  /// Unblocks all threads currently waiting on this.
  void broadcast();
  /// @}

private:
  // Fixed-capacity storage for any OS condition variable type, adjust if
  // necessary.
  alignas(16) Byte m_cond[64];
};

inline void ConditionVariable::wait(ScopeLock<Mutex>& _scope_lock) {
  wait(_scope_lock.m_lock);
}

inline void ConditionVariable::wait(ScopeLock<RecursiveMutex>& _scope_lock) {
  wait(_scope_lock.m_lock);
}

template<typename P>
void ConditionVariable::wait(Mutex& _mutex, P&& _predicate) {
  while (!_predicate()) {
    wait(_mutex);
  }
}

template<typename P>
void ConditionVariable::wait(RecursiveMutex& _mutex, P&& _predicate) {
  while (!_predicate()) {
    wait(_mutex);
  }
}

template<typename P>
void ConditionVariable::wait(ScopeLock<Mutex>& _scope_lock, P&& _predicate) {
  while (!_predicate()) {
    wait(_scope_lock);
  }
}

template<typename P>
void ConditionVariable::wait(ScopeLock<RecursiveMutex>& _scope_lock, P&& _predicate) {
  while (!_predicate()) {
    wait(_scope_lock);
  }
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H
