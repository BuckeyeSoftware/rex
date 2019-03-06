#if __has_include(<windows.h>)
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
#define yield() SwitchToThread()
#elif __has_include(<sched.h>)
#include <sched.h> // sched_yield
#define yield() sched_yield()
#endif

#include <rx/core/concurrency/spin_lock.h> // spin_lock

static constexpr const int k_backoff_spins{10000};

// ThreadSanitizer annotations
#if defined(RX_TSAN)
extern "C"
{
  void __tsan_acquire(void*);
  void __tsan_release(void*);
};
# define tsan_acquire(x) __tsan_acquire(x)
# define tsan_release(x) __tsan_release(x)
#else
# define tsan_acquire(x)
# define tsan_release(x)
#endif

namespace rx::concurrency {

void spin_lock::lock() {
  tsan_acquire(&m_lock);

  for (;;) {
    for (int i{0}; i < k_backoff_spins; i++) {
      if (__sync_bool_compare_and_swap(&m_lock, 0, 1)) {
        return;
      }
    }
    yield();
  }
}

void spin_lock::unlock() {
  __asm__ __volatile__("" ::: "memory");
  m_lock = 0;
  tsan_release(&m_lock);
}

} // namespace rx::concurrency
