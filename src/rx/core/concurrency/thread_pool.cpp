#include "rx/core/concurrency/thread_pool.h"
#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"
#include "rx/core/concurrency/thread.h"

#include "rx/core/time/stop_watch.h"

#include "rx/core/memory/slab.h"

#include "rx/core/log.h"

namespace Rx::Concurrency {

RX_LOG("ThreadPool", logger);

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

struct ThreadPool::Impl {
  RX_MARK_NO_COPY(Impl);
  RX_MARK_NO_MOVE(Impl);

  Memory::Allocator& allocator;
  Mutex mutex;
  ConditionVariable task_cond;
  ConditionVariable ready_cond;
  IntrusiveList queue       RX_HINT_GUARDED_BY(mutex);
  Vector<Thread> threads    RX_HINT_GUARDED_BY(mutex);
  Memory::Slab job_memory   RX_HINT_GUARDED_BY(mutex);
  bool stop                 RX_HINT_GUARDED_BY(mutex);
  Time::StopWatch           timer;
  Concurrency::Atomic<Size> ready;
  Concurrency::Atomic<Size> active_threads;

  Impl(Memory::Allocator& _allocator)
    : allocator{_allocator}
    , threads{_allocator}
    , stop{false}
    , ready{0}
    , active_threads{0}
  {
  }

  ~Impl() {
    Time::StopWatch timer;
    timer.start();

    {
      ScopeLock lock{mutex};
      stop = true;
    }

    task_cond.broadcast();

    threads.each_fwd([](Thread &_thread) {
      RX_ASSERT(_thread.join(), "failed to join thread");
    });

    timer.stop();

    logger->verbose("stopped pool with %zu threads (took %s)",
      threads.size(), timer.elapsed());
  }

  bool add_task(ThreadPool::Task&& task_) {
    {
      ScopeLock lock{mutex};
      auto item = job_memory.create<Work>(Utility::move(task_));
      if (!item) {
        logger->error("out of memory");
        return false;
      }
      queue.push_back(&item->link);
    }
    task_cond.signal();
    return true;
  }

  bool init(Size _threads, Size _pool_size) {
    auto slab = Memory::Slab::create(allocator, sizeof(Work), _pool_size, 1, 0);
    if (!slab) {
      logger->error("out of memory");
      return false;
    }

    job_memory = Utility::move(*slab);

    timer.start();

    logger->info("starting pool with %zu threads", _threads);

    auto thread_func = [_threads, this](int _thread_id) {
      logger->info("starting thread %d", _thread_id);

      // When all threads are started.
      if (++ready == _threads) {
        timer.stop();
        logger->info("started pool with %zu threads (took %s)", _threads,
          timer.elapsed());
      }

      for (;;) {
        Function<void(int)> task;
        {
          ScopeLock lock{mutex};

          task_cond.wait(lock, [this] { return stop || !queue.is_empty(); });
          if (stop && queue.is_empty()) {
            logger->info("stopping thread %d", _thread_id);
            return;
          }

          auto node = queue.pop_back();
          auto item = node->data<Work>(&Work::link);

          task = Utility::move(item->callback);

          job_memory.destroy(item);
        }

        active_threads++;
        logger->verbose("starting task on thread %d", _thread_id);

        Time::StopWatch timer;
        timer.start();
        task(_thread_id);
        timer.stop();

        logger->verbose("finished task on thread %d (took %s)",
          _thread_id, timer.elapsed());
        active_threads--;
      }
    };

    // Create the threads.
    if (!threads.reserve(_threads)) {
      return false;
    }

    for (Size i = 0; i < _threads; i++) {
      auto func = Task::create(thread_func);
      if (!func) {
        return false;
      }
      auto thread = Thread::create(allocator, "thread pool", Utility::move(*func));
      if (!thread || !threads.push_back(Utility::move(*thread))) {
        return false;
      }
    }

    return true;
  }
};

Optional<ThreadPool> ThreadPool::create(Memory::Allocator& _allocator, Size _threads,
  Size _pool_size)
{
  auto impl = _allocator.create<Impl>(_allocator);
  if (!impl || !impl->init(_threads, _pool_size)) {
    _allocator.destroy<Impl>(impl);
    return nullopt;
  }

  return ThreadPool { _allocator, impl };
}

ThreadPool::~ThreadPool() {
  m_allocator->destroy<Impl>(m_impl);
}

bool ThreadPool::add_task(Task&& task_) {
  return m_impl ? m_impl->add_task(Utility::move(task_)) : false;
}

Size ThreadPool::total_threads() const {
  return m_impl ? m_impl->threads.size() : 0;
}

Size ThreadPool::active_threads() const {
  return m_impl ? m_impl->active_threads.load() : 0;
}

} // namespace Rx::Concurrency
