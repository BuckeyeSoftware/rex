#ifndef RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H
#define RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H

#include <pthread.h> // pthread_cond_{t, wait, signal}

#include <rx/core/assert.h> // RX_ASSERT

#include <rx/core/concurrency/scope_lock.h> // spin_lock
#include <rx/core/concurrency/mutex.h> // mutex

namespace rx::concurrency {

struct condition_variable {
  condition_variable();
  ~condition_variable();

  void wait(mutex& _mutex);
  void wait(scope_lock<mutex>& _scope_lock);

  template<typename P>
  void wait(mutex& _mutex, P&& _predicate) {
    while (!_predicate()) {
      wait(_mutex);
    }
  }

  template<typename P>
  void wait(scope_lock<mutex>& _scope_lock, P&& _predicate) {
    while (!_predicate()) {
      wait(_scope_lock);
    }
  }

  void signal();
  void broadcast();

private:
  pthread_cond_t m_cond;
};

inline condition_variable::condition_variable() {
  if (pthread_cond_init(&m_cond, nullptr) != 0) {
    RX_ASSERT(false, "failed to initialize");
  }
}

inline condition_variable::~condition_variable() {
  if (pthread_cond_destroy(&m_cond) != 0) {
    RX_ASSERT(false, "failed to destroy");
  }
}

inline void condition_variable::wait(mutex& _mutex) {
  if (pthread_cond_wait(&m_cond, &_mutex.m_mutex) != 0) {
    RX_ASSERT(false, "failed to wait");
  }
}

inline void condition_variable::wait(scope_lock<mutex>& _scope_lock) {
  wait(_scope_lock.m_lock);
}

inline void condition_variable::signal() {
  if (pthread_cond_signal(&m_cond) != 0) {
    RX_ASSERT(false, "failed to signal");
  }
}

inline void condition_variable::broadcast() {
  if (pthread_cond_broadcast(&m_cond) != 0) {
    RX_ASSERT(false, "failed to broadcast");
  }
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H
