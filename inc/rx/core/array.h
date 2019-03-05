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
  constexpr array(memory::allocator* alloc);

  template<typename... Ts>
  array(memory::allocator* al, rx_size size, Ts&&... args);
  array(const array& other);
  array(array&& other);
  ~array();

  T& operator[](rx_size i);
  const T& operator[](rx_size i) const;

  // resize to |size| with |value| for new objects
  bool resize(rx_size size, const T& value = {});

  // reserve |size| elements
  bool reserve(rx_size size);

  void clear();

  // append |data| by copy
  bool push_back(const T& data);
  // append |data| by move
  bool push_back(T&& data);

  // append new |T| construct with |args|
  template<typename... Ts>
  bool emplace_back(Ts&&... args);

  rx_size size() const;
  bool is_empty() const;

  // enumerate collection either forward or reverse
  template<typename F>
  bool each_fwd(F&& func);
  template<typename F>
  bool each_rev(F&& func);
  template<typename F>
  bool each_fwd(F&& func) const;
  template<typename F>
  bool each_rev(F&& func) const;

  // first or last element
  const T& first() const;
  T& first();
  const T& last() const;
  T& last();

  const T* data() const;
  T* data();

  T&& pop_back() {
    T&& data{utility::move(last())};
    utility::destruct<T>(this->data() + (m_size - 1));
    m_size--;
    return utility::move(data);
  }

private:
  // NOTE(dweiler): does not adjust m_size just adjusts capacity
  bool grow_or_shrink_to(rx_size size);

  memory::allocator* m_allocator;
  memory::block m_data;
  rx_size m_size;
  rx_size m_capacity;
};

template<typename T>
inline constexpr array<T>::array()
  : array{&memory::g_system_allocator}
{
}

template<typename T>
inline constexpr array<T>::array(memory::allocator* alloc)
  : m_allocator{alloc}
  , m_data{}
  , m_size{0}
  , m_capacity{0}
{
}

template<typename T>
template<typename... Ts>
inline array<T>::array(memory::allocator* alloc, rx_size size, Ts&&... args)
  : m_allocator{alloc}
  , m_data{alloc->allocate(size * sizeof(T))}
  , m_size{size}
  , m_capacity{size}
{
  RX_ASSERT(m_data, "out of memory");

  for (rx_size i{0}; i < m_capacity; i++) {
    utility::construct<T>(m_data.cast<T*>() + i, utility::forward<Ts>(args)...);
  }
}

template<typename T>
inline array<T>::array(const array& other)
  : m_allocator{other.m_allocator}
  , m_data{m_allocator->allocate(other.m_capacity * sizeof(T))}
  , m_size{other.m_size}
  , m_capacity{other.m_capacity}
{
  RX_ASSERT(m_data, "out of memory");

  for (rx_size i{0}; i < m_size; i++) {
    utility::construct<T>(m_data.cast<T*>() + i, other[i]);
  }
}

template<typename T>
inline array<T>::array(array&& other)
  : m_allocator{other.m_allocator}
  , m_data{utility::move(other.m_data)}
  , m_size{other.m_size}
  , m_capacity{other.m_capacity}
{
  other.m_allocator = nullptr;
  other.m_size = 0;
  other.m_capacity = 0;
}

template<typename T>
inline array<T>::~array() {
  // clear();
  // if (m_allocator) {
  //   m_allocator->deallocate(utility::move(m_data));
  // }
}

template<typename T>
inline T& array<T>::operator[](rx_size i) {
  RX_ASSERT(i < m_size, "out of bounds (%zu >= %zu)", i, m_size);
  return m_data.cast<T*>()[i];
}

template<typename T>
inline const T& array<T>::operator[](rx_size i) const {
  RX_ASSERT(i < m_size, "out of bounds (%zu >= %zu)", i, m_size);
  return m_data.cast<T*>()[i];
}

template<typename T>
bool array<T>::grow_or_shrink_to(rx_size size) {
  if (!reserve(size)) {
    return false;
  }

  if constexpr (!traits::is_trivially_destructible<T>) {
    if (size < m_size) {
      for (rx_size i{m_size-1}; i > size; i--) {
        utility::destruct<T>(data() + i);
      }
    }
  }

  return true;
}

template<typename T>
bool array<T>::resize(rx_size size, const T& value) {
  if (!grow_or_shrink_to(size)) {
    return false;
  }

  // copy construct new objects
  for (rx_size i{m_size}; i < size; i++) {
    utility::construct<T>(data() + i, value);
  }

  m_size = size;
  return true;
}

template<typename T>
bool array<T>::reserve(rx_size size) {
  if (size <= m_capacity) {
    return true;
  }

  // golden ratio
  while (m_capacity < size) {
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
inline bool array<T>::emplace_back(Ts&&... args) {
  if (!grow_or_shrink_to(m_size + 1)) {
    return false;
  }

  // forward construct object
  utility::construct<T>(data() + m_size, utility::forward<Ts>(args)...);

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
inline bool array<T>::each_fwd(F&& func) {
  for (rx_size i{0}; i < m_size; i++) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!func(operator[](i))) {
        return false;
      }
    } else {
      func(operator[](i));
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool array<T>::each_fwd(F&& func) const {
  return const_cast<array<T>*>(this)->each_fwd(func);
}

template<typename T>
template<typename F>
inline bool array<T>::each_rev(F&& func) {
  for (rx_size i{m_size-1}; i < m_size; i--) {
    if constexpr (traits::is_same<traits::return_type<F>, bool>) {
      if (!func(operator[](i))) {
        return false;
      }
    } else {
      func(operator[](i));
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool array<T>::each_rev(F&& func) const {
  return const_cast<array<T>*>(this)->each_rev(func);
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
