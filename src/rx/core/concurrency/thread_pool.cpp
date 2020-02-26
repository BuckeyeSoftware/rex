#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/concurrency/wait_group.h"

#include "rx/core/time/stopwatch.h"
#include "rx/core/time/delay.h"
#include "rx/core/log.h"

namespace rx::concurrency {

RX_LOG("thread_pool", logger);
RX_GLOBAL<thread_pool> thread_pool::s_thread_pool{"system", "thread_pool", 4_z, 4096_z};

struct work {
  work(function<void(int)>&& _callback)
    : callback{utility::move(_callback)}
  {
  }

  intrusive_list::node link;
  function<void(int)> callback;
};

thread_pool::thread_pool(memory::allocator* _allocator, rx_size _threads, rx_size _static_pool_size)
  : m_allocator{_allocator}
  , m_threads{m_allocator}
  , m_job_memory{m_allocator, sizeof(work), _static_pool_size}
  , m_stop{false}
{
  time::stopwatch timer;
  timer.start();

  logger(log::level::k_info, "starting pool with %zu threads", _threads);
  m_threads.reserve(_threads);

  wait_group group{_threads};
  for (rx_size i{0}; i < _threads; i++) {
    m_threads.emplace_back("thread pool", [this, &group](int _thread_id) {
      logger(log::level::k_info, "starting thread %d", _thread_id);

      group.signal();

      for (;;) {
        function<void(int)> task;
        {
          scope_lock lock{m_mutex};
          m_task_cond.wait(lock, [this] { return m_stop || !m_queue.is_empty(); });
          if (m_stop && m_queue.is_empty()) {
            logger(log::level::k_info, "stopping thread %d", _thread_id);
            return;
          }

          auto node = m_queue.pop_back();
          auto item = node->data<work>(&work::link);

          task = utility::move(item->callback);

          m_job_memory.destroy(item);
        }

        logger(log::level::k_verbose, "starting task on thread %d", _thread_id);

        time::stopwatch timer;
        timer.start();
        task(_thread_id);
        timer.stop();

        logger(log::level::k_verbose, "finished task on thread %d (took %s)",
          _thread_id, timer.elapsed());
      }
    });
  }

  // wait for all threads to start
  group.wait();

  timer.stop();
  logger(log::level::k_info, "started pool with %zu threads (took %s)", _threads,
    timer.elapsed());
}

thread_pool::~thread_pool() {
  time::stopwatch timer;
  timer.start();
  {
    scope_lock lock{m_mutex};
    m_stop = true;
  }
  m_task_cond.broadcast();

  m_threads.each_fwd([](thread &_thread) {
    _thread.join();
  });

  timer.stop();
  logger(log::level::k_verbose, "stopped pool with %zu threads (took %s)",
    m_threads.size(), timer.elapsed());
}

void thread_pool::add(function<void(int)>&& task_) {
  {
    scope_lock lock{m_mutex};
    auto item = m_job_memory.create<work>(utility::move(task_));
    m_queue.push_back(&item->link);
  }
  m_task_cond.signal();
}

} // namespace rx::concurrency
