#include <rx/core/concurrency/thread.h>
#include <rx/core/concurrency/spin_lock.h>
#include <rx/core/concurrency/scope_lock.h>

#include <rx/core/memory/system_allocator.h>

namespace rx::concurrency {

static spin_lock g_lock;
static int g_thread_id; // protected by |g_lock|

thread::thread()
  : m_allocator{nullptr}
{
  // {empty}
}

thread::thread(thread&& _thread)
  : m_allocator{_thread.m_allocator}
  , m_state{utility::move(_thread.m_state)}
{
  _thread.m_allocator = nullptr;
}

thread::thread(rx::memory::allocator* _allocator, rx::function<void(int)>&& _function)
  : m_allocator{_allocator}
{
  RX_ASSERT(m_allocator, "null allocator");
  m_state = utility::move(m_allocator->allocate(sizeof(state)));
  utility::construct<state>(m_state.data(), utility::move(_function));
}

thread::thread(rx::function<void(int)>&& _function)
  : thread{&rx::memory::g_system_allocator, utility::move(_function)}
{
  // {empty}
}

thread::~thread() {
  if (m_state) {
    m_state.cast<state*>()->join();
    utility::destruct<state>(m_state.data());

    RX_ASSERT(m_allocator, "null allocator");
    m_allocator->deallocate(utility::move(m_state));
  }
}

void thread::join() {
  if (m_state) {
    m_state.cast<state*>()->join();
  }
}

// state
void* thread::state::wrap(void* _data) {
  int thread_id = 0;
  {
    scope_lock<spin_lock> locked(g_lock);
    thread_id = g_thread_id++;
  }

  auto self{reinterpret_cast<state*>(_data)};
  self->m_function(utility::move(thread_id));
  return nullptr;
}

thread::state::state()
  : m_joined{false}
{
  // {empty}
}

thread::state::state(rx::function<void(int)>&& _function)
  : m_function{utility::move(_function)}
  , m_joined{false}
{
  if (pthread_create(&m_thread, nullptr, wrap, reinterpret_cast<void*>(this)) != 0) {
    RX_ASSERT(false, "thread creation failed");
  }
}

void thread::state::join() {
  if (m_joined) {
    return;
  }

  if (pthread_join(m_thread, nullptr) != 0) {
    RX_ASSERT(false, "join failed");
  }

  m_joined = true;
}

} // namespace rx::concurrency
