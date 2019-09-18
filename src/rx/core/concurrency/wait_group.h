#ifndef RX_CORE_CONCURRENCY_WAIT_GROUP_H
#define RX_CORE_CONCURRENCY_WAIT_GROUP_H
#include "rx/core/types.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"

namespace rx::concurrency {

struct wait_group {
  wait_group();

  void signal();
  void wait(rx_size _count);

private:
  rx_size m_count; // protected by |m_mutex|
  mutex m_mutex;
  condition_variable m_condition_variable;
};

inline wait_group::wait_group()
  : m_count{0}
{
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_WAIT_GROUP_H