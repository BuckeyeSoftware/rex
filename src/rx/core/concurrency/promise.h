#ifndef RX_CORE_CONCURRENCY_PROMISE_H
#define RX_CORE_CONCURRENCY_PROMISE_H
#include "rx/core/concurrency/atomic.h"
#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"

#include "rx/core/memory/allocator.h"
#include "rx/core/memory/uninitialized_storage.h"

#include <stdlib.h> // malloc

namespace Rx::Concurrency {

struct PromiseState {
  static inline constexpr const auto INSITU_ALIGNMENT = Memory::Allocator::ALIGNMENT;
  static inline constexpr const auto INSITU_SIZE = 64 * 4; // 4 cache lines.

  PromiseState()
    : m_ready{false}
    , m_reference_count{1}
    , m_destructor{nullptr}
  {
  }

  PromiseState* acquire() {
    m_reference_count++;
    return this;
  }

  void release() {
    if (--m_reference_count == 0) {
      destroy();
    }
  }

  bool is_ready() const {
    return m_ready;
  }

  template<typename T>
  void write(T&& value_) {
    Concurrency::ScopeLock lock{m_lock};
    Utility::construct<T>(m_data.data(), Utility::move(value_));
    m_destructor = &Utility::destruct<T>;
    m_ready = true;
    m_cond.signal();
  }

  template<typename T>
  T* read() {
    // Check the atomic to avoid holding a lock.
    if (!m_ready) {
      Concurrency::ScopeLock lock{m_lock};
      m_cond.wait(lock, [this] { return m_ready.load(); });
    }
    return reinterpret_cast<T*>(m_data.data());
  }

private:
  void destroy() {
    m_lock.lock();
    if (m_destructor) {
      m_destructor(m_data.data());
    }
    m_lock.unlock();
    free(this);
  }

  // Keep |m_data| at the top to keep it hot.
  Memory::UninitializedStorage<INSITU_SIZE, INSITU_ALIGNMENT> m_data;

  Concurrency::Atomic<Bool> m_ready;           // 4
  Concurrency::Atomic<Size> m_reference_count; // 4 or 8
  Concurrency::Mutex m_lock;                   // 16
  Concurrency::ConditionVariable m_cond;       // 16
  void (*m_destructor)(void*);                 // 4 or 8
};

static constexpr const auto S = sizeof(PromiseState);


template<typename T>
struct Promise {
  static_assert(sizeof(T) <= PromiseState::INSITU_SIZE, "too large");

  constexpr Promise()
    : m_state{Utility::construct<PromiseState>(malloc(sizeof(PromiseState)))}
  {
  }

  ~Promise() { release(); }

  Promise(Promise&& move_)
    : m_state{Utility::exchange(move_.m_state, nullptr)}
  {
  }

  Promise(const Promise& _copy)
    : m_state{_copy.acquire()}
  {
  }

  Promise& operator=(Promise&& move_) {
    release();
    m_state = Utility::exchange(move_.m_state, nullptr);
    return *this;
  }

  Promise& operator=(const Promise& _copy) {
    release();
    m_state = _copy.acquire();
    return *this;
  }

  bool is_ready() const {
    return m_state ? m_state->is_ready() : false;
  }

  T* value() const {
    return m_state ? m_state->read<T>() : nullptr;
  }

  // Only called from ThreadPool.
  bool signal(T&& value_) const {
    if (is_ready()) {
      return false;
    }
    m_state->write<T>(Utility::move(value_));
    return true;
  }

private:
  PromiseState* acquire() const { return m_state ? m_state->acquire() : nullptr; }
  void release() { if (m_state) m_state->release(); }

  PromiseState* m_state;
};

} // namespace Rx::Concurrency

#endif
