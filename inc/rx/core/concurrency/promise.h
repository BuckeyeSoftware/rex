#ifndef RX_CORE_CONCURRENCY_PROMISE_H
#define RX_CORE_CONCURRENCY_PROMISE_H

#include <rx/core/concurrency/mutex.h>
#include <rx/core/concurrency/condition_variable.h>
#include <rx/core/concurrency/scope_lock.h>

#include <rx/core/utility/move.h>
#include <rx/core/utility/construct.h>
#include <rx/core/utility/destruct.h>

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
  memory::block m_state;
};

template<typename T>
inline promise<T>::state::state()
  : m_done{false}
{
  // {empty}
}

template<typename T>
inline promise<T>::promise(rx::memory::allocator* _allocator)
  : m_allocator{_allocator}
{
  RX_ASSERT(m_allocator, "null allocator");
}

template<typename T>
inline promise<T>::promise()
  : promise{&rx::memory::g_system_allocator}
{
  m_state = utility::move(m_allocator->allocate(sizeof(state)));
  utility::construct<state>(m_state.data());
}

template<typename T>
inline promise<T>::~promise() {
  if (m_state) {
    {
      auto state_data{m_state.cast<state*>()};
      scope_lock<mutex> lock(state_data->m_mutex);
      state_data->m_condition_variable.wait(lock, [state_data] { return state_data->m_done; });
    }

    utility::destruct<state>(m_state.data());
    m_allocator->deallocate(utility::move(m_state));
  }
}

template<typename T>
inline promise<T>::promise(promise&& _promise)
  : m_allocator{_promise.m_allocator}
  , m_state{utility::move(_promise.m_state)}
{
  // {empty}
}

template<typename T>
inline void promise<T>::set_value(const T& _value) {
  auto state_data{m_state.cast<state*>()};
  {
    scope_lock<mutex> lock(state_data->m_mutex);
    state_data->m_result = _value;
    state_data->m_done = true;
    state_data->m_condition_variable.signal();
  }
}

template<typename T>
inline void promise<T>::set_value(T&& _value) {
  auto state_data{m_state.cast<state*>()};
  {
    scope_lock<mutex> lock(state_data->m_mutex);
    state_data->m_result = utility::move(_value);
    state_data->m_done = true;
    state_data->m_condition_variable.signal();
  }
}

template<typename T>
inline T promise<T>::get_value() const {
  auto state_data{m_state.cast<state*>()};
  scope_lock<mutex> lock(state_data->m_mutex);
  state_data->m_condition_variable.wait(lock, [state_data] { return state_data->m_done; });
  return state_data->m_result;
}

} // namespace rx::concurrency

#endif
