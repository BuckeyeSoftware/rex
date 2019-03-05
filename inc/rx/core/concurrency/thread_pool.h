#ifndef RX_CORE_CONCURRENCY_THREAD_POOL_H
#define RX_CORE_CONCURRENCY_THREAD_POOL_H

#include <rx/core/queue.h>
#include <rx/core/function.h>

#include <rx/core/concurrency/thread.h>
#include <rx/core/concurrency/mutex.h>
#include <rx/core/concurrency/condition_variable.h>
#include <rx/core/concurrency/scope_lock.h>

namespace rx::concurrency {

struct thread_pool {
  thread_pool(rx_size threads = 16)
    : m_stop{false}
  {
    for (rx_size i{0}; i < threads; i++) {
      m_threads.emplace_back([this](int thread_id) {
        for (;;) {
          rx::function<void(int)> task;
          {
            scope_lock<mutex> lock(m_mutex);
            m_task_cond.wait(lock, [this] { return m_stop || !m_queue.is_empty(); });
            if (m_stop && m_queue.is_empty()) {
              return;
            }
            task = utility::move(m_queue.pop());
          }
          task(utility::move(thread_id));
        }
      });
    }
  }

  ~thread_pool() {
    {
      scope_lock<mutex> lock(m_mutex);
      m_stop = true;
    }
    m_task_cond.broadcast();

    m_threads.each_fwd([](thread &_thread) {
      _thread.join();
    });
  }

  void add(rx::function<void(int)>&& _task) {
    {
      scope_lock<mutex> lock(m_mutex);
      m_queue.push(utility::move(_task));
    }
    m_task_cond.signal();
  }

private:
  bool m_stop; // protected by |m_mutex|
  mutex m_mutex;
  condition_variable m_task_cond;
  queue<rx::function<void(int)>> m_queue; // protected by |m_mutex|
  array<thread> m_threads;
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_POOL_H
