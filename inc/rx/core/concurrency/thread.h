#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H

#include <pthread.h>

#include <rx/core/function.h>
#include <rx/core/concepts/no_copy.h>
#include <rx/core/memory/allocator.h>

namespace rx::concurrency {

struct thread : concepts::no_copy {
  thread();

  // construct thread with system allocator and function
  thread(rx::function<void(int)>&& function);
  // construct thread with custom allocator and function
  thread(rx::memory::allocator* m_allocator, rx::function<void(int)>&& function);

  // move construct thread
  thread(thread&& _thread);

  ~thread();

  void join();

private:
  struct state {
    static void* wrap(void* data);

    state();
    state(rx::function<void(int)>&& _function);

    void join();

    pthread_t m_thread;
    rx::function<void(int)> m_function;
    bool m_joined;
  };

  rx::memory::allocator* m_allocator;
  rx::memory::block m_state;
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
