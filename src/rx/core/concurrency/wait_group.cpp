#include "rx/core/concurrency/wait_group.h"
#include "rx/core/concurrency/scope_lock.h"

namespace rx::concurrency {

void wait_group::signal() {
  scope_lock lock{m_mutex};
  m_count++;
  m_condition_variable.signal();
}

void wait_group::wait(rx_size _count) {
  scope_lock lock{m_mutex};
  m_condition_variable.wait(lock, [&]{ return _count == m_count; });
}

} // namespace rx::concurrency