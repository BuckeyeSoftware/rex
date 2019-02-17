#include <rx/core/concurrency/spin_lock.h> // spin_lock

namespace rx::concurrency {

void spin_lock::lock() {
  for (;;) {
    if (__sync_bool_compare_and_swap(&m_lock, 0, 1)) {
      return;
    }
  }
}

void spin_lock::unlock() {
  __asm__ __volatile__("" ::: "memory");
  m_lock = 0;
}

} // namespace rx::concurrency
