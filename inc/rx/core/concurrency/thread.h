#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H

#include <pthread.h> // pthread_t, pthread_{create, join}

#include <rx/core/traits.h> // conditional, declval, move, nat

namespace rx::concurrency {

template<typename F, typename T>
struct thread {
  using return_type = decltype(declval<F>()({}));
  thread(F&& function, T* data);
  ~thread();
  return_type join();
  void launch();

private:
  static void* thread_wrap(void* data);
  pthread_t m_thread;
  F&& m_function;
  return_type m_result;
  T* m_data;
  bool m_joined;
  bool m_launched;
};

// template deduction guide
template<typename F, typename T>
thread(F&& function, T* data) -> thread<F, T>;

template<typename F, typename T>
inline thread<F, T>::thread(F&& function, T* data)
  : m_function{move(function)}
  , m_result{}
  , m_data{data}
  , m_joined{false}
  , m_launched{false}
{
}

template<typename F, typename T>
inline thread<F, T>::~thread() {
  if (m_launched && !m_joined) {
    join();
  }
}

template<typename F, typename T>
inline typename thread<F, T>::return_type thread<F, T>::join() {
  if (pthread_join(m_thread, nullptr) != 0) {
    RX_ASSERT(false, "join failed");
  }
  m_joined = true;
  return m_result;
}

template<typename F, typename T>
inline void thread<F, T>::launch() {
  m_launched = true;
  if (pthread_create(&m_thread, nullptr, thread_wrap, reinterpret_cast<void*>(this)) != 0) {
    RX_ASSERT(false, "thread creation failed");
  }
}

template<typename F, typename T>
void* thread<F, T>::thread_wrap(void* data) {
  auto self = reinterpret_cast<thread<F, T>*>(data);
  self->m_result = move(self->m_function(self->m_data));
  return nullptr;
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H
