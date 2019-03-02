#ifndef RX_CORE_CONCURRENCY_MUTEX_H
#define RX_CORE_CONCURRENCY_MUTEX_H

#include <pthread.h> // pthread_mutex_t, pthread_mutex_{init, destroy, lock, unlock}

#include <rx/core/assert.h> // RX_ASSERT

namespace rx::concurrency {

struct mutex {
  mutex();
  ~mutex();

  void lock();
  void unlock();

private:
  friend class condition_variable;
  pthread_mutex_t m_mutex;
};

inline mutex::mutex() {
  if (pthread_mutex_init(&m_mutex, nullptr) != 0) {
    RX_ASSERT(false, "initialization failed");
  }
}

inline mutex::~mutex() {
  if (pthread_mutex_destroy(&m_mutex) != 0) {
    RX_ASSERT(false, "destroy failed");
  }
}

inline void mutex::lock() {
  if (pthread_mutex_lock(&m_mutex) != 0) {
    RX_ASSERT(false, "lock failed");
  }
}

inline void mutex::unlock() {
  if (pthread_mutex_unlock(&m_mutex) != 0) {
    RX_ASSERT(false, "unlock failed");
  }
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_MUTEX_H
