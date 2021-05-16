#ifndef RX_CORE_CONCURRENCY_THREAD_POOL_H
#define RX_CORE_CONCURRENCY_THREAD_POOL_H
#include "rx/core/intrusive_list.h"

#include "rx/core/concurrency/scheduler.h"
#include "rx/core/concurrency/thread.h"
#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"

#include "rx/core/time/stop_watch.h"

#include "rx/core/memory/slab.h"

namespace Rx::Concurrency {

struct RX_API ThreadPool
  : Scheduler
{
  RX_MARK_NO_COPY(ThreadPool);
  RX_MARK_NO_MOVE(ThreadPool);

  ThreadPool(Memory::Allocator& _allocator, Size _threads, Size _static_pool_size);
  ThreadPool(Size _threads, Size _job_pool_size);
  ~ThreadPool();

  [[nodiscard]] virtual bool add_task(Task&& task_);

  virtual Size total_threads() const;
  virtual Size active_threads() const;

  constexpr Memory::Allocator& allocator() const;

  static constexpr ThreadPool& instance();

private:
  Memory::Allocator& m_allocator;

  Mutex m_mutex;
  ConditionVariable m_task_cond;
  ConditionVariable m_ready_cond;

  IntrusiveList m_queue     RX_HINT_GUARDED_BY(m_mutex);
  Vector<Thread> m_threads  RX_HINT_GUARDED_BY(m_mutex);
  Memory::Slab m_job_memory RX_HINT_GUARDED_BY(m_mutex);
  bool m_stop               RX_HINT_GUARDED_BY(m_mutex);
  Time::StopWatch m_timer;
  Concurrency::Atomic<Size> m_ready;
  Concurrency::Atomic<Size> m_active_threads;

  static Global<ThreadPool> s_instance;
};

inline ThreadPool::ThreadPool(Size _threads, Size _static_pool_size)
  : ThreadPool{Memory::SystemAllocator::instance(), _threads, _static_pool_size}
{
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& ThreadPool::allocator() const {
  return m_allocator;
}

RX_HINT_FORCE_INLINE constexpr ThreadPool& ThreadPool::instance() {
  return *s_instance;
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_POOL_H
