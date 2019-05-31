#ifndef RX_CORE_CONCURRENCY_SCOPE_LOCK_H
#define RX_CORE_CONCURRENCY_SCOPE_LOCK_H

namespace rx::concurrency {

// generic scoped lock
template<typename T>
struct scope_lock {
  explicit constexpr scope_lock(T& lock);
  ~scope_lock();
private:
  friend struct condition_variable;
  T& m_lock;
};

template<typename T>
inline constexpr scope_lock<T>::scope_lock(T& lock)
  : m_lock{lock}
{
  m_lock.lock();
}

template<typename T>
inline scope_lock<T>::~scope_lock() {
  m_lock.unlock();
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_SCOPE_LOCK_H
