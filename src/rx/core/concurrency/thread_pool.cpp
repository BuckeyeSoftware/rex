#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/concurrency/wait_group.h"

#include "rx/core/time/stop_watch.h"
#include "rx/core/time/delay.h"

#include "rx/core/log.h"

namespace Rx::Concurrency {

RX_LOG("ThreadPool", logger);

Global<ThreadPool> ThreadPool::s_instance{"system", "thread_pool", 4_z, 4096_z};

struct Work {
  RX_MARK_NO_COPY(Work);
  RX_MARK_NO_MOVE(Work);

  Work(Function<void(int)>&& callback_)
    : callback{Utility::move(callback_)}
  {
  }

  IntrusiveList::Node link;
  Function<void(int)> callback;
};

ThreadPool::ThreadPool(Memory::Allocator& _allocator, Size _threads, Size _static_pool_size)
  : m_allocator{_allocator}
  , m_threads{allocator()}
  , m_job_memory{allocator(), sizeof(Work), _static_pool_size}
  , m_stop{false}
{
  Time::StopWatch timer;
  timer.start();

  logger->info("starting pool with %zu threads", _threads);
  RX_ASSERT(m_threads.reserve(_threads), "out of memory");

  WaitGroup group{_threads};
  for (Size i{0}; i < _threads; i++) {
    auto thread_func = Task::create([this, &group](int _thread_id) {
      logger->info("starting thread %d", _thread_id);

      group.signal();

      for (;;) {
        Function<void(int)> task;
        {
          ScopeLock lock{m_mutex};
          m_task_cond.wait(lock, [this] { return m_stop || !m_queue.is_empty(); });
          if (m_stop && m_queue.is_empty()) {
            logger->info("stopping thread %d", _thread_id);
            return;
          }

          auto node = m_queue.pop_back();
          auto item = node->data<Work>(&Work::link);

          task = Utility::move(item->callback);

          m_job_memory.destroy(item);
        }

        logger->verbose("starting task on thread %d", _thread_id);

        Time::StopWatch timer;
        timer.start();
        task(_thread_id);
        timer.stop();

        logger->verbose("finished task on thread %d (took %s)",
          _thread_id, timer.elapsed());
      }
    });

    if (!thread_func || !m_threads.emplace_back("thread pool", Utility::move(*thread_func))) {
      break;
    }
  }

  // wait for all threads to start
  group.wait();

  timer.stop();
  logger->info("started pool with %zu threads (took %s)", _threads,
    timer.elapsed());
}

ThreadPool::~ThreadPool() {
  Time::StopWatch timer;
  timer.start();
  {
    ScopeLock lock{m_mutex};
    m_stop = true;
  }
  m_task_cond.broadcast();

  m_threads.each_fwd([](Thread &_thread) {
    _thread.join();
  });

  timer.stop();
  logger->verbose("stopped pool with %zu threads (took %s)",
    m_threads.size(), timer.elapsed());
}

bool ThreadPool::insert(Function<void(int)>&& task_) {
  {
    ScopeLock lock{m_mutex};
    auto item = m_job_memory.create<Work>(Utility::move(task_));
    if (!item) {
      return false;
    }
    m_queue.push_back(&item->link);
  }

  m_task_cond.signal();

  return true;
}

} // namespace Rx::Concurrency
