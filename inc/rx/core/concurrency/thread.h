#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H

#include <pthread.h> // pthread_t, pthread_{create, join}

#include <rx/core/traits.h> // conditional, declval, move, nat

namespace rx::concurrency {

template<typename F>
struct thread {
  using return_type = decltype(declval<F>()());
  thread(F&& function);
  ~thread();
  return_type join();

private:
  static void* thread_wrap(void* data);
  pthread_t m_thread;
  F&& m_function;
  conditional<is_same<return_type, void>, nat, return_type> m_result;
  bool m_joined;
};

// template deduction guide
template<typename F>
thread(F&& function) -> thread<F>;

template<typename F>
inline thread<F>::thread(F&& function)
  : m_function{move(function)}
  , m_result{}
  , m_joined{false}
{
  if (pthread_create(&m_thread, nullptr, thread_wrap, reinterpret_cast<void*>(this)) != 0) {
    RX_ASSERT(false, "thread creation failed");
  }
}

template<typename F>
inline thread<F>::~thread() {
  if (!m_joined) {
    join();
  }
}

template<typename F>
inline typename thread<F>::return_type thread<F>::join() {
  if (pthread_join(m_thread, nullptr) != 0) {
    RX_ASSERT(false, "join failed");
  }

  m_joined = true;

  if constexpr(!is_same<return_type, void>) {
    return m_result;
  }
}

template<typename F>
void* thread<F>::thread_wrap(void* data) {
  auto self = reinterpret_cast<thread<F>*>(data);
  if constexpr(!is_same<return_type, void>) {
    self->m_result = move(self->m_function());
  } else {
    self->m_function();
  }
  return nullptr;
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
