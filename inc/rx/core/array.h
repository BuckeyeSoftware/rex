#ifndef RX_CORE_ARRAY_H
#define RX_CORE_ARRAY_H

#include <rx/core/assert.h> // RX_ASSERT

#include <rx/core/traits/is_same.h>
#include <rx/core/traits/is_trivially_copyable.h>
#include <rx/core/traits/is_trivially_destructible.h>
#include <rx/core/traits/return_type.h>

#include <rx/core/utility/forward.h>
#include <rx/core/utility/move.h>
#include <rx/core/utility/construct.h>
#include <rx/core/utility/destruct.h>

#include <rx/core/memory/system_allocator.h> // memory::{system_allocator, allocator, block}

namespace rx {

// 32-bit: 20 bytes
// 64-bit: 40 bytes
template<typename T>
struct array {
  constexpr array();
  constexpr array(memory::allocator* _allocator);

  template<typename... Ts>
  array(memory::allocator* _allocator, rx_size _size, Ts&&... _args);
  array(const array& _other);
  array(array&& _other);
  ~array();

  array& operator=(const array& _other);

  T& operator[](rx_size _index);
  const T& operator[](rx_size _index) const;

  // resize to |size| with |value| for new objects
  bool resize(rx_size _size, const T& _value = {});

  // reserve |size| elements
  bool reserve(rx_size _size);

  void clear();

  // append |data| by copy
  bool push_back(const T& _data);
  // append |data| by move
  bool push_back(T&& _data);

  // append new |T| construct with |args|
  template<typename... Ts>
  bool emplace_back(Ts&&... _args);

  rx_size size() const;
  bool is_empty() const;

  // enumerate collection either forward or reverse
  template<typename F>
  bool each_fwd(F&& _func);
  template<typename F>
  bool each_rev(F&& _func);
  template<typename F>
  bool each_fwd(F&& _func) const;
  template<typename F>
  bool each_rev(F&& _func) const;

  // first or last element
  const T& first() const;
  T& first();
  const T& last() const;
  T& last();

  const T* data() const;
  T* data();

private:
  // NOTE(dweiler): does not adjust m_size just adjusts capacity
  bool grow_or_shrink_to(rx_size _size);

  memory::allocator* m_allocator;
  memory::block m_data;
  rx_size m_size;
  rx_size m_capacity;
};

template<typename T>
inline constexpr array<T>::array()
  : array{&memory::g_system_allocator}
{
  // {empty}
}

template<typename T>
inline constexpr array<T>::array(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_data{}
  , m_size{0}
  , m_capacity{0}
{
  RX_ASSERT(m_allocator, "null allocator");
}

template<typename T>
template<typename... Ts>
inline array<T>::array(memory::allocator* _allocator, rx_size _size, Ts&&... _args)
  : m_allocator{_allocator}
  , m_size{_size}
  , m_capacity{_size}
{
  RX_ASSERT(m_allocator, "null allocator");

  m_data = utility::move(m_allocator->allocate(_size * sizeof(T)));
  RX_ASSERT(m_data, "out of memory");

  for (rx_size i{0}; i < m_capacity; i++) {
    utility::construct<T>(data() + i, utility::forward<Ts>(_args)...);
  }
}

template<typename T>
inline array<T>::array(const array& _other)
  : m_allocator{_other.m_allocator}
  , m_data{m_allocator->allocate(_other.m_capacity * sizeof(T))}
  , m_size{_other.m_size}
  , m_capacity{_other.m_capacity}
{
  RX_ASSERT(m_data, "out of memory");

  for (rx_size i{0}; i < m_size; i++) {
    utility::construct<T>(data() + i, _other[i]);
  }
}

template<typename T>
inline array<T>::array(array&& _other)
  : m_allocator{_other.m_allocator}
  , m_data{utility::move(_other.m_data)}
  , m_size{_other.m_size}
  , m_capacity{_other.m_capacity}
{
  _other.m_size = 0;
  _other.m_capacity = 0;
}

template<typename T>
inline array<T>::~array() {
  clear();
  m_allocator->deallocate(utility::move(m_data));
}

template<typename T>
inline array<T>& array<T>::operator=(const array& _other) {
  clear();

  if (_other.is_empty()) {
    return *this;
  }

  m_allocator = _other.m_allocator;
  m_data = m_allocator->allocate(_other.m_capacity * sizeof(T));
  m_size = _other.m_size;
  m_capacity = _other.m_capacity;
  RX_ASSERT(m_data, "out of memory");

  for (rx_size i{0}; i < m_size; i++) {
    utility::construct<T>(data() + i, _other[i]);
  }

  return *this;
}

template<typename T>
inline T& array<T>::operator[](rx_size _index) {
  RX_ASSERT(_index < m_size, "out of bounds (%zu >= %zu)", _index, m_size);
  return data()[_index];
}

template<typename T>
inline const T& array<T>::operator[](rx_size _index) const {
  RX_ASSERT(_index < m_size, "out of bounds (%zu >= %zu)", _index, m_size);
  return data()[_index];
}

template<typename T>
bool array<T>::grow_or_shrink_to(rx_size _size) {
  if (!reserve(_size)) {
    return false;
  }

  if constexpr (!traits::is_trivially_destructible<T>) {
    if (_size < m_size) {
      for (rx_size i{m_size-1}; i > _size; i--) {
        utility::destruct<T>(data() + i);
      }
    }
  }

  return true;
}

template<typename T>
bool array<T>::resize(rx_size _size, const T& _value) {
  if (!grow_or_shrink_to(_size)) {
    return false;
  }

  // copy construct new objects
  for (rx_size i{m_size}; i < _size; i++) {
    utility::construct<T>(data() + i, _value);
  }

  m_size = _size;
  return true;
}

template<typename T>
bool array<T>::reserve(rx_size _size) {
  if (_size <= m_capacity) {
    return true;
  }

  // golden ratio
  while (m_capacity < _size) {
    m_capacity = ((m_capacity + 1) * 3) / 2;
  }

  if constexpr (traits::is_trivially_copyable<T>) {
    memory::block resize{m_allocator->reallocate(m_data, m_capacity * sizeof(T))};
    if (resize) {
      m_data = utility::move(resize);
      return true;
    }
  } else {
    memory::block resize{m_allocator->allocate(m_capacity * sizeof(T))};
    if (resize) {
      if (m_size) {
        auto *const src{m_data.cast<T*>()};
        auto *const dst{resize.cast<T*>()};
        for (rx_size i{0}; i < m_size; i++) {
          utility::construct<T>(dst + i, utility::move(*(src + i)));
          utility::destruct<T>(src + i);
        }
      }

      m_allocator->deallocate(utility::move(m_data));
      m_data = utility::move(resize);
      return true;
    }
  }

  return false;
}

template<typename T>
inline void array<T>::clear() {
  if (m_size) {
    if constexpr (!traits::is_trivially_destructible<T>) {
      for (rx_size i{m_size-1}; i < m_size; i--) {
        utility::destruct<T>(data() + i);
      }
    }
  }
  m_size = 0;
}

template<typename T>
inline bool array<T>::push_back(const T& _value) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // copy construct object
  utility::construct<T>(data() + m_size, _value);

  m_size++;
  return true;
}

template<typename T>
inline bool array<T>::push_back(T&& _value) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // move construct object
  utility::construct<T>(data() + m_size, utility::move(_value));

  m_size++;
  return true;
}

template<typename T>
template<typename... Ts>
inline bool array<T>::emplace_back(Ts&&... _args) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // forward construct object
  utility::construct<T>(data() + m_size, utility::forward<Ts>(_args)...);

  m_size++;
  return true;
}

template<typename T>
inline rx_size array<T>::size() const {
  return m_size;
}

template<typename T>
inline bool array<T>::is_empty() const {
  return m_size == 0;
}

template<typename T>
template<typename F>
inline bool array<T>::each_fwd(F&& _func) {
  for (rx_size i{0}; i < m_size; i++) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(operator[](i))) {
        return false;
      }
    } else {
      _func(operator[](i));
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool array<T>::each_fwd(F&& _func) const {
  for (rx_size i{0}; i < m_size; i++) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(operator[](i))) {
        return false;
      }
    } else {
      _func(operator[](i));
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool array<T>::each_rev(F&& _func) {
  for (rx_size i{m_size-1}; i < m_size; i--) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(operator[](i))) {
        return false;
      }
    } else {
      _func(operator[](i));
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool array<T>::each_rev(F&& _func) const {
  for (rx_size i{m_size-1}; i < m_size; i--) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!_func(operator[](i))) {
        return false;
      }
    } else {
      _func(operator[](i));
    }
  }
  return true;
}

template<typename T>
inline const T& array<T>::last() const {
  return data()[m_size - 1];
}

template<typename T>
inline T& array<T>::last() {
  return data()[m_size - 1];
}

template<typename T>
const T* array<T>::data() const {
  return m_data.cast<const T*>();
}

template<typename T>
inline T* array<T>::data() {
  return m_data.cast<T*>();
}

} // namespace rx

#endif // RX_CORE_ARRAY_H
