#ifndef RX_CORE_EVENT_H
#define RX_CORE_EVENT_H

#include "rx/core/function.h"

namespace rx {

template<typename T>
struct event {
  using delegate = function<void(T)>;

  struct handle {
    constexpr handle(event* _event, rx_size _index);
    ~handle();
  private:
    event* m_event;
    rx_size m_index;
  };

  constexpr event();
  constexpr event(memory::allocator* _allocator);

  void signal(const T& _value);
  handle connect(delegate&& _function);

private:
  friend struct handle;
  array<delegate> m_delegates;
};

template<typename T>
inline constexpr event<T>::handle::handle(event<T>* _event, rx_size _index)
  : m_event{_event}
  , m_index{_index}
{
}

template<typename T>
inline event<T>::handle::~handle() {
  m_event->m_delegates[m_index] = nullptr;
}

template<typename T>
inline constexpr event<T>::event()
  : event{&memory::g_system_allocator}
{
}

template<typename T>
inline constexpr event<T>::event(memory::allocator* _allocator)
  : m_delegates{_allocator}
{
}

template<typename T>
inline void event<T>::signal(const T& _value) {
  m_delegates.each_fwd([&_value](delegate& _delegate) {
    if (_delegate) {
      _delegate(_value);
    }
  });
}

template<typename T>
inline typename event<T>::handle event<T>::connect(delegate&& _delegate) {
  const rx_size delegates{m_delegates.size()};
  for (rx_size i{0}; i < delegates; i++) {
    if (!m_delegates[i]) {
      return {this, i};
    }
  }
  m_delegates.emplace_back(utility::move(_delegate));
  return {this, delegates};
}

} // namespace rx

#endif // RX_CORE_EVENT_H