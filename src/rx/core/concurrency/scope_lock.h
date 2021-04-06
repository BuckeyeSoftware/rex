#ifndef RX_CORE_CONCURRENCY_SCOPE_LOCK_H
#define RX_CORE_CONCURRENCY_SCOPE_LOCK_H
#include "rx/core/markers.h"

/// \file scope_lock

namespace Rx::Concurrency {

/// \brief Scope-based lockable ownership wrapper.
///
/// Provides a convenient RAII-style mechanism for owning a lockable for the
/// duration of a scoped block.
///
/// When a ScopeLock object is created, it attempts to take ownership of
/// the lockable it is given. When control leaves the scope in which the
/// ScopeLock object was created, the ScopeLock is destructed and the
/// lockable is released.
template<typename T>
struct ScopeLock {
  RX_MARK_NO_COPY(ScopeLock);

  /// Constructs a ScopeLock, locking the given lockable.
  /// \param lock_ The lockable to take ownership of.
  explicit constexpr ScopeLock(T& lock_);

  /// Destructs the ScopeLock, unlocking the underlying lockable.
  ~ScopeLock();

private:
  friend struct ConditionVariable;
  T& m_lock;
};

template<typename T>
constexpr ScopeLock<T>::ScopeLock(T& lock_)
  : m_lock{lock_}
{
  m_lock.lock();
}

template<typename T>
ScopeLock<T>::~ScopeLock() {
  m_lock.unlock();
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_SCOPE_LOCK_H
