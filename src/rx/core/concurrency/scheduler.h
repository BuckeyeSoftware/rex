#ifndef RX_CORE_CONCURRENCY_SCHEDULER_H
#define RX_CORE_CONCURRENCY_SCHEDULER_H
#include "rx/core/function.h"

/// \file scheduler.h

namespace Rx::Concurrency {

/// \brief Interface for describing a scheduler.
///
/// The scheduler interface allows implementing simple task-based schedulers
/// like thread pools as a polymorphic thing.
struct RX_API Scheduler {
  /// The task type that add_task() expects.
  using Task = Function<void(Sint32)>;

  virtual ~Scheduler() = default;

  /// Helper routine to add an invocable to the scheduler.
  ///
  /// This helper takes any invocable type (function, functor, lambda, etc)
  /// and constructs a Task from it and calls add_task().
  ///
  /// \param functor_ The functor to add.
  template<typename F>
  [[nodiscard]] bool add(F&& functor_);

  /// Total number of threads.
  virtual Size total_threads() const = 0;
  /// Number of threads that are currently executing tasks.
  virtual Size active_threads() const = 0;

  /// The function which must be implemented to add a task to the scheduler.
  /// \param task_ The task to add.
  [[nodiscard]] virtual bool add_task(Task&& task_) = 0;
};

template<typename F>
bool Scheduler::add(F&& functor_) {
  if (auto task = Task::create(Utility::forward<F>(functor_))) {
    return add_task(Utility::move(*task));
  }
  return false;
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_SCHEDULER_H