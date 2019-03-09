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
  thread_pool(rx_size threads = 16)
    : m_stop{false}
    , m_ready{0}
  {
    RX_MESSAGE("starting thread pool with %zu threads", threads);

    for (rx_size i{0}; i < threads; i++) {
      m_threads.emplace_back([this](int _thread_id) {
        RX_MESSAGE("starting thread %d for pool", _thread_id);

        {
          scope_lock lock(m_mutex);
          m_ready++;
          m_ready_cond.signal();
        }

        for (;;) {
          function<void(int)> task;
          {
            scope_lock lock(m_mutex);
            m_task_cond.wait(lock, [this] { return m_stop || !m_queue.is_empty(); });
            if (m_stop && m_queue.is_empty()) {
              RX_MESSAGE("stopping thread %d for pool", _thread_id);
              return;
            }
            task = utility::move(m_queue.pop());
          }
          task(utility::move(_thread_id));
        }
      });
    }

    // wait for all threads to start
    {
      scope_lock lock(m_mutex);
      m_ready_cond.wait(lock, [this] { return m_ready == m_threads.size(); });
    }

    RX_MESSAGE("all threads started");
  }

  ~thread_pool() {
    {
      scope_lock lock(m_mutex);
      m_stop = true;
    }
    m_task_cond.broadcast();

    m_threads.each_fwd([](thread &_thread) {
      _thread.join();
    });

    RX_MESSAGE("all threads stopped");
  }

  void add(function<void(int)>&& _task) {
    {
      scope_lock lock(m_mutex);
      m_queue.push(utility::move(_task));
    }
    m_task_cond.signal();
  }

private:
  mutex m_mutex;
  condition_variable m_task_cond;
  condition_variable m_ready_cond;
  queue<function<void(int)>> m_queue; // protected by |m_mutex|
  array<thread> m_threads; // protected by |m_mutex|
  bool m_stop; // protected by |m_mutex|
  rx_size m_ready; // protected by |m_mutex|
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_POOL_H
