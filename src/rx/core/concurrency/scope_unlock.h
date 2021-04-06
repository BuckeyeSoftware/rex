#ifndef RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H
#define RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H

/// \file scope_unlock.h

namespace Rx::Concurrency {

/// \brief Unlocked scope.
///
/// Provides a convenient RAII-style mechanism for unlocking a lockable for the
/// duration of a scoped block.
///
/// When a ScopeUnlock object is created, it attempts to release ownership of
/// the lockable it is given. When control leaves the scope in which the
/// ScopeUnlock object was created, the ScopeLock is destructed and the
/// lockable is reacquired (relocked).
template<typename T>
struct ScopeUnlock {
  /// Constructs a ScopeUnlock, unlocking the lockable.
  explicit constexpr ScopeUnlock(T& lock_);

  /// Destructs the ScopeUnlock, locking the lockable.
  ~ScopeUnlock();

private:
  T& m_lock;
};

template<typename T>
constexpr ScopeUnlock<T>::ScopeUnlock(T& lock_)
  : m_lock{lock_}
{
  m_lock.unlock();
}

template<typename T>
ScopeUnlock<T>::~ScopeUnlock() {
  m_lock.lock();
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H
