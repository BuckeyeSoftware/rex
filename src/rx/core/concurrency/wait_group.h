#ifndef RX_CORE_CONCURRENCY_WAIT_GROUP_H
#define RX_CORE_CONCURRENCY_WAIT_GROUP_H
#include "rx/core/types.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"

/// \file wait_group.h

namespace Rx::Concurrency {

/// \brief Convenience type to wait for a group of concurrent tasks.
struct RX_API WaitGroup {
  /// \brief Construct a wait group
  /// \param _count The number of things intended to be waited on.
  WaitGroup(Size _count);
  WaitGroup();

  /// \brief Signal completion of one thing in the group.
  /// \return A boolean value indicating if there are more things to wait for.
  bool signal();

  /// \brief Blocks calling thread until all things in group are signaled.
  void wait();

private:
  Size m_signaled_count RX_HINT_GUARDED_BY(m_mutex);
  Size m_count          RX_HINT_GUARDED_BY(m_mutex);
  Mutex m_mutex;
  ConditionVariable m_condition_variable;
};

inline WaitGroup::WaitGroup(Size _count)
  : m_signaled_count{0}
  , m_count{_count}
{
}

inline WaitGroup::WaitGroup()
  : WaitGroup{0}
{
}

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_WAIT_GROUP_H
