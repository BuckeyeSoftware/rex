#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H
#include "rx/core/function.h"
#include "rx/core/ptr.h"

#include "rx/core/hints/empty_bases.h"

namespace rx::concurrency {

// NOTE: Thread names must have static-storage which lives as long as the thread.
// NOTR: Cannot deliver signals to threads.
struct RX_HINT_EMPTY_BASES thread
  : concepts::no_copy
{
  template<typename F>
  thread(memory::allocator* _allocator, const char* _name, F&& _function);

  template<typename F>
  thread(const char* _name, F&& _function);

  thread(thread&& thread_);
  ~thread();

  thread& operator=(thread&& thread_) = delete;

  void join();

  memory::allocator* allocator() const;

private:
  struct state {
    template<typename F>
    state(memory::allocator* _allocator, const char* _name, F&& _function);

    void spawn();
    void join();

  private:
    static void* wrap(void* _data);

    union {
      utility::nat m_nat;
      // Fixed-capacity storage for any OS thread type, adjust if necessary.
      alignas(16) rx_byte m_thread[16];
    };

    function<void(int)> m_function;
    const char* m_name;
    bool m_joined;
  };

  ptr<state> m_state;
};

template<typename F>
inline thread::state::state(memory::allocator* _allocator, const char* _name, F&& _function)
  : m_nat{}
  , m_function{_allocator, utility::forward<F>(_function)}
  , m_name{_name}
  , m_joined{false}
{
  spawn();
}

template<typename F>
inline thread::thread(memory::allocator* _allocator, const char* _name, F&& _function)
  : m_state{make_ptr<state>(_allocator, _allocator, _name, utility::forward<F>(_function))}
{
}

template<typename F>
inline thread::thread(const char* _name, F&& _function)
  : thread{&memory::g_system_allocator, _name, utility::forward<F>(_function)}
{
}

inline thread::thread(thread&& thread_)
  : m_state{utility::move(thread_.m_state)}
{
}

inline memory::allocator* thread::allocator() const {
  return m_state.allocator();
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
