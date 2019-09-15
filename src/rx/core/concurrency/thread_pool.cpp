#include <SDL.h>

#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/log.h"

namespace rx::concurrency {

RX_LOG("thread_pool", logger);

thread_pool::thread_pool(memory::allocator* _allocator, rx_size _threads)
  : m_allocator{_allocator}
  , m_queue{m_allocator}
  , m_threads{m_allocator}
  , m_stop{false}
  , m_ready{0}
{
  logger(log::level::k_info, "starting pool with %zu threads", _threads);
  const auto beg{SDL_GetPerformanceCounter()};
  m_threads.reserve(_threads);

  for (rx_size i{0}; i < _threads; i++) {
    m_threads.emplace_back("thread pool", [this](int _thread_id) {
      logger(log::level::k_info, "starting thread %d", _thread_id);

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
            logger(log::level::k_info, "stopping thread %d", _thread_id);
            return;
          }
          task = m_queue.pop();
        }

        logger(log::level::k_verbose, "starting task on thread %d", _thread_id);
        const auto beg{SDL_GetPerformanceCounter()};
        task(_thread_id);
        const auto end{SDL_GetPerformanceCounter()};
        const auto time{static_cast<rx_f64>(((end - beg) * 1000.0) / static_cast<rx_f64>(SDL_GetPerformanceFrequency()))};
        logger(log::level::k_verbose, "finished task on thread %d (took %.2f ms)", _thread_id, time);
      }
    });
  }

  // wait for all threads to start
  {
    scope_lock lock(m_mutex);
    m_ready_cond.wait(lock, [this] { return m_ready == m_threads.size(); });
  }

  const auto end{SDL_GetPerformanceCounter()};
  const auto time{static_cast<rx_f64>(((end - beg) * 1000.0) / static_cast<rx_f64>(SDL_GetPerformanceFrequency()))};
  logger(log::level::k_info, "started pool with %zu threads (took %.2f ms)", _threads, time);
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
}

void thread_pool::add(function<void(int)>&& _task) {
  {
    scope_lock lock(m_mutex);
    m_queue.push(utility::move(_task));
  }
  m_task_cond.signal();
}

} // namespace rx::concurrency