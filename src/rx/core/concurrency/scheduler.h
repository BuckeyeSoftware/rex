#ifndef RX_CORE_CONCURRENCY_SCHEDULER_H
#define RX_CORE_CONCURRENCY_SCHEDULER_H
#include "rx/core/function.h"

namespace Rx::Concurrency {

struct Scheduler {
  using Task = Function<void(Sint32)>;

  virtual ~Scheduler() = default;

  template<typename F>
  [[nodiscard]] bool add(F&& functor_);

  [[nodiscard]] virtual bool add_task(Task&& task_) = 0;
};

template<typename F>
bool Scheduler::add(F&& functor_) {
  if (auto task = Task::create(Utility::move(functor_))) {
    return add_task(Utility::move(*task));
  }
  return false;
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_SCHEDULER_H