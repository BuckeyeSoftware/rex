#ifndef RX_CORE_CONCURRENCY_THREAD_POOL_H
#define RX_CORE_CONCURRENCY_THREAD_POOL_H
#include "rx/core/concurrency/scheduler.h"

namespace Rx::Concurrency {

/// \brief Pool of threads
///
/// Thread pool models the Scheduler interface and provides a multi-threaded
/// system for adding tasks and having them execute on background threads when
/// those threads are available.
///
/// The queue for the pool has a fixed initial capacity (pool size), adding more
/// tasks than this initial capacity is permitted, but doing so expands the
/// idle memory usage of the pool. Setting a good estimate for how many tasks
/// will be queued at max avoids invoking the allocator, reducing memory
/// fragmentation and lock contention. The latter is the primary reason for this
/// design, as tasks are created and destroyed on different threads.
///
/// The actual design of this fixed initial capacity of tasks uses the same
/// behavior of the slab in Memory::Slab.
struct RX_API ThreadPool
  : Scheduler
{
  RX_MARK_NO_COPY(ThreadPool);

  constexpr ThreadPool();
  ThreadPool(ThreadPool&& thread_pool_);
  ~ThreadPool();

  ///\brief Create a thread pool
  ///
  /// \param _allocator Allocator to use for operations.
  /// \param _threads Number of threads in the pool.
  /// \param _pool_size Number of work items to reserve for the pool.
  ///
  /// \return The thread pool on success, nullopt otherwise. This function can
  /// fail when out of memory.
  static Optional<ThreadPool> create(Memory::Allocator& _allocator, Size _threads,
    Size _pool_size);

  ThreadPool& operator=(ThreadPool&& thread_pool_);

  /// \brief Add task to the pool.
  /// \param task_ The task to add.
  [[nodiscard]] virtual bool add_task(Task&& task_);

  /// \brief Total number of threads in the thread pool.
  virtual Size total_threads() const;

  /// \brief Number of threads currently occupied with work.
  virtual Size active_threads() const;

  /// \brief Allocator used to construct the pool.
  constexpr Memory::Allocator& allocator() const;

private:
  // Use a private implementation because the ThreadPool needs to be movable.
  struct Impl;

  constexpr ThreadPool(Memory::Allocator& _allocator, Impl* _impl);

  Memory::Allocator* m_allocator;
  Impl* m_impl;
};

inline constexpr ThreadPool::ThreadPool()
  : m_allocator{&Memory::NullAllocator::instance()}
  , m_impl{nullptr}
{
}

inline constexpr ThreadPool::ThreadPool(Memory::Allocator& _allocator, Impl* _impl)
  : m_allocator{&_allocator}
  , m_impl{_impl}
{
}

inline ThreadPool::ThreadPool(ThreadPool&& thread_pool_)
  : m_allocator{Utility::exchange(thread_pool_.m_allocator, &Memory::NullAllocator::instance())}
  , m_impl{Utility::exchange(thread_pool_.m_impl, nullptr)}
{
}

inline ThreadPool& ThreadPool::operator=(ThreadPool&& thread_pool_) {
  if (this != &thread_pool_) {
    m_allocator = Utility::exchange(thread_pool_.m_allocator, &Memory::NullAllocator::instance());
    m_impl = Utility::exchange(thread_pool_.m_impl, nullptr);
  }
  return *this;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& ThreadPool::allocator() const {
  return *m_allocator;
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_POOL_H
