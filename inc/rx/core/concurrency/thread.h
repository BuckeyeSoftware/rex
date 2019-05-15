#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H

#include <rx/core/config.h> // RX_PLATFORM_*

#if defined(RX_PLATFORM_POSIX)
#include <pthread.h>
#elif defined(RX_PLATFORM_WINDOWS)
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#undef interface
#else
#error "missing thread implementation"
#endif

#include <rx/core/function.h>
#include <rx/core/concepts/no_copy.h>
#include <rx/core/memory/allocator.h>

namespace rx::concurrency {

// NOTE: thread names must be static strings
struct thread : concepts::no_copy {
  thread();

  thread(const char* _name, function<void(int)>&& _function);
  thread(memory::allocator* _allocator, const char* _name, function<void(int)>&& _function);
  thread(thread&& _thread);
  ~thread();

  void join();

private:
  struct state {
    static void* wrap(void* data);

    state();
    state(const char* _name, function<void(int)>&& _function);

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
  state* m_state;
};

inline thread::thread(const char* _name, function<void(int)>&& _function)
  : thread{&memory::g_system_allocator, _name, utility::move(_function)}
{
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
