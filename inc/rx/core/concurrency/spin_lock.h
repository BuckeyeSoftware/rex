#ifndef RX_CORE_CONCURRENCY_SPIN_LOCK_H
#define RX_CORE_CONCURRENCY_SPIN_LOCK_H

#include <rx/core/types.h>

namespace rx::concurrency {

struct spin_lock {
  constexpr spin_lock();
  ~spin_lock() = default;
  void lock();
  void unlock();
private:
  rx_u32 m_lock;
};

inline constexpr spin_lock::spin_lock()
  : m_lock{0}
{
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_SPIN_LOCK_H
