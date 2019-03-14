#ifndef RX_CORE_CONCURRENCY_THREAD_POOL_H
#define RX_CORE_CONCURRENCY_THREAD_POOL_H

#include <rx/core/queue.h>
#include <rx/core/function.h>

#include <rx/core/concurrency/thread.h>
#include <rx/core/concurrency/mutex.h>
#include <rx/core/concurrency/spin_lock.h>
#include <rx/core/concurrency/condition_variable.h>
#include <rx/core/concurrency/scope_lock.h>

namespace rx::concurrency {

struct thread_pool {
  thread_pool(rx_size _threads = 16);
  ~thread_pool();

  // insert |_task| into the thread pool to be executed, the integer passed
  // to |_task| is the thread id of the calling thread in the pool
  void add(function<void(int)>&& _task);
  
private:
  mutex m_mutex;
  condition_variable m_task_cond;
  condition_variable m_ready_cond;
  queue<function<void(int)>> m_queue; // protected by |m_mutex|
  array<thread> m_threads;            // protected by |m_mutex|
  bool m_stop;                        // protected by |m_mutex|
  rx_size m_ready;                    // protected by |m_mutex|
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_POOL_H
