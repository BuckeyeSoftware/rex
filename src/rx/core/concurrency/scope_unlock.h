#ifndef RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H
#define RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H

namespace Rx::Concurrency {

// generic scoped unlock
template<typename T>
struct ScopeUnlock {
  explicit constexpr ScopeUnlock(T& lock_);
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
