#ifndef RX_CORE_CONCURRENCY_PROMISE_H
#define RX_CORE_CONCURRENCY_PROMISE_H

#include <rx/core/concurrency/mutex.h>
#include <rx/core/concurrency/condition_variable.h>
#include <rx/core/concurrency/scope_lock.h>

#include <rx/core/utility/move.h>
#include <rx/core/utility/construct.h>
#include <rx/core/utility/destruct.h>

#include <rx/core/memory/allocator.h>

#include <rx/core/concepts/no_copy.h>

namespace rx::concurrency {

template<typename T>
struct promise : concepts::no_copy {
  promise();
  promise(memory::allocator* _allocator);
  ~promise();

  promise(promise&& _promise);

  void set_value(const T& _value);
  void set_value(T&& _value);

  T get_value() const;

private:
  struct state {
    state();
    T m_result;
    mutable mutex m_mutex;
    mutable condition_variable m_condition_variable;
    bool m_done;
  };

  memory::allocator* m_allocator;
  state* m_state;
};

template<typename T>
inline promise<T>::state::state()
  : m_done{false}
{
}

template<typename T>
inline promise<T>::promise(memory::allocator* _allocator)
  : m_allocator{_allocator}
{
  RX_ASSERT(m_allocator, "null allocator");

  m_state = utility::allocate_and_construct<state>(m_allocator);
  RX_ASSERT(m_state, "out of memory");
}

template<typename T>
inline promise<T>::promise()
  : promise{&memory::g_system_allocator}
{
}

template<typename T>
inline promise<T>::~promise() {
  if (m_state) {
    {
      scope_lock lock(m_state->m_mutex);
      m_state->m_condition_variable.wait(lock, [m_state] { return m_state->m_done; });
    }
    utility::destruct_and_deallocate<state>(m_allocator, m_state);
  }
}

template<typename T>
inline promise<T>::promise(promise&& _promise)
  : m_allocator{_promise.m_allocator}
  , m_state{_promise.m_state}
{
  _promise.m_allocator = nullptr;
  _promise.m_state = nullptr;
}

template<typename T>
inline void promise<T>::set_value(const T& _value) {
  scope_lock lock(m_state->m_mutex);
  m_state->m_result = _value;
  m_state->m_done = true;
  m_state->m_condition_variable.signal();
}

template<typename T>
inline void promise<T>::set_value(T&& _value) {
  scope_lock lock(m_state->m_mutex);
  m_state->m_result = utility::move(_value);
  m_state->m_done = true;
  m_state->m_condition_variable.signal();
}

template<typename T>
inline T promise<T>::get_value() const {
  scope_lock lock(m_state->m_mutex);
  m_state->m_condition_variable.wait(lock, [m_state] { return m_state->m_done; });
  return m_state->m_result;
}

} // namespace rx::concurrency

#endif
