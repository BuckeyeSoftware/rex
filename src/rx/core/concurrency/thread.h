#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H
#include "rx/core/function.h"

namespace rx::concurrency {

// Thread names must have static-storage which lives as long as the thread.
struct thread
  : concepts::no_copy
{
  constexpr thread();

  thread(memory::allocator* _allocator, const char* _name, function<void(int)>&& function_);
  thread(const char* _name, function<void(int)>&& function_);
  thread(thread&& thread_);
  ~thread();

  void join();

  memory::allocator* allocator() const;

private:
  struct state {
    static void* wrap(void* data);

    state();
    state(const char* _name, function<void(int)>&& function_);

    void join();

    // Fixed-capacity storage for any OS thread type, adjust if necessary.
    alignas(16) rx_byte m_thread[16];

    function<void(int)> m_function;
    bool m_joined;
    const char* m_name;
  };

  memory::allocator* m_allocator;
  state* m_state;
};

inline constexpr thread::thread()
  : m_allocator{nullptr}
  , m_state{nullptr}
{
}

inline thread::thread(const char* _name, function<void(int)>&& function_)
  : thread{&memory::g_system_allocator, _name, utility::move(function_)}
{
}

inline thread::thread(thread&& thread_)
  : m_allocator{thread_.m_allocator}
  , m_state{thread_.m_state}
{
  thread_.m_allocator = nullptr;
  thread_.m_state = nullptr;
}

inline memory::allocator* thread::allocator() const {
  return m_allocator;
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
