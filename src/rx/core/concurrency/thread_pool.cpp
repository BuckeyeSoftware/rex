#include <rx/core/concurrency/thread_pool.h>

namespace rx::concurrency {

thread_pool::thread_pool(rx_size _threads)
  : m_stop{false}
  , m_ready{0}
{
  RX_MESSAGE("starting thread pool with %zu threads", _threads);

  for (rx_size i{0}; i < _threads; i++) {
    m_threads.emplace_back("thread pool", [this](int _thread_id) {
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

thread_pool::~thread_pool() {
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

void thread_pool::add(function<void(int)>&& _task) {
  {
    scope_lock lock(m_mutex);
    m_queue.push(utility::move(_task));
  }
  m_task_cond.signal();
}

} // namespace rx::concurrency