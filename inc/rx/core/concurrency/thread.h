#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H

#if defined(RX_PLATFORM_POSIX)
#include <pthread.h>
#elif defined(RX_PLATFORM_WINDOWS)
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#error "missing thread implementation"
#endif

#include <rx/core/function.h>
#include <rx/core/concepts/no_copy.h>
#include <rx/core/memory/allocator.h>

namespace rx::concurrency {

struct thread : concepts::no_copy {
  thread();

  // construct thread with system allocator and function
  thread(function<void(int)>&& _function);
  // construct thread with custom allocator and function
  thread(memory::allocator* _allocator, function<void(int)>&& _function);

  // move construct thread
  thread(thread&& _thread);

  ~thread();

  void join();

private:
  struct state {
    static void* wrap(void* data);

    state();
    state(function<void(int)>&& _function);

    void join();

#if defined(RX_PLATFORM_POSIX)
    pthread_t m_thread;
#elif defined(RX_PLATFORM_WINDOWS)
    HANDLE m_thread;
#endif

    function<void(int)> m_function;
    bool m_joined;
  };

  memory::allocator* m_allocator;
  memory::block m_state;
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
