#ifndef RX_CORE_ARRAY_H
#define RX_CORE_ARRAY_H

#include <rx/core/traits.h> // is_{same, trivially_{copyable, destructible}}
#include <rx/core/assert.h> // RX_ASSERT

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

  template<typename... Ts>
  bool resize(rx_size size, Ts&&... args);
  bool reserve(rx_size size);
  void clear();

  bool push_back(const T& data);
  bool push_back(T&& data);

  template<typename... Ts>
  bool emplace_back(Ts&&... args);

  rx_size size() const;

  template<typename F>
  bool each(F&& func);

  template<typename F>
  bool each(F&& func) const;

private:
  memory::allocator* m_allocator;
  memory::block m_data;
  rx_size m_size;
  rx_size m_capacity;
};

template<typename T>
inline constexpr array<T>::array()
  : array{&*memory::g_system_allocator}
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
    call_ctor<T>(m_data.cast<T*>() + i, forward<Ts>(args)...);
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
    call_ctor<T>(m_data.cast<T*>() + i, other[i]);
  }
}

template<typename T>
inline array<T>::array(array&& other)
  : m_allocator{other.m_allocator}
  , m_data{move(other.m_data)}
  , m_size{other.m_size}
  , m_capacity{other.m_capacity}
{
}

template<typename T>
inline array<T>::~array() {
  m_allocator->deallocate(move(m_data));
}

template<typename T>
inline T& array<T>::operator[](rx_size i) {
  RX_ASSERT(i < m_size, "out of bounds");
  return m_data.cast<T*>()[i];
}

template<typename T>
inline const T& array<T>::operator[](rx_size i) const {
  RX_ASSERT(i < m_size, "out of bounds");
  return m_data.cast<T*>()[i];
}

template<typename T>
template<typename... Ts>
bool array<T>::resize(rx_size size, Ts&&... args) {
  if (!reserve(size)) {
    return false;
  }

  auto *const data{m_data.cast<T*>()};

  if constexpr (!is_trivially_destructible<T>) {
    if (size < m_size) {
      for (rx_size i{m_size-1}; i > size; i--) {
        call_dtor<T>(data + i);
      }
    }
  }

  for (rx_size i{m_size}; i < size; i++) {
    call_ctor<T>(data + i, forward<Ts>(args)...);
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

  if constexpr (is_trivially_copyable<T>) {
    memory::block resize{m_allocator->reallocate(move(m_data), m_capacity * sizeof(T))};
    if (resize) {
      m_data = move(resize);
      return true;
    }
  } else {
    memory::block resize{m_allocator->allocate(m_capacity * sizeof(T))};
    if (resize) {
      auto *const src{m_data.cast<T*>()};
      auto *const dst{resize.cast<T*>()};
      for (rx_size i{0}; i < m_size; i++) {
        call_ctor<T>(dst + i, move(*(src + i)));
        call_dtor<T>(src + i);
      }

      m_allocator->deallocate(move(m_data));
      m_data = move(resize);
      return true;
    }
  }

  return false;
}

template<typename T>
inline void array<T>::clear() {
  if constexpr (!is_trivially_destructible<T>) {
    auto *const data{m_data.cast<T*>()};
    for (rx_size i{m_size-1}; i < m_size; i--) {
      call_dtor(data + i);
    }
  }
  m_size = 0;
}

template<typename T>
inline bool array<T>::push_back(const T& data) {
  const auto size{m_size};
  if (resize(size + 1)) {
    operator[](size) = data;
    return true;
  }
  return false;
}

template<typename T>
inline bool array<T>::push_back(T&& data) {
  const auto size{m_size};
  if (resize(size + 1)) {
    operator[](size) = move(data);
    return true;
  }
  return false;
}

template<typename T>
template<typename... Ts>
inline bool array<T>::emplace_back(Ts&&... args) {
  return resize(m_size + 1, forward<Ts>(args)...);
}

template<typename T>
inline rx_size array<T>::size() const {
  return m_size;
}

template<typename T>
template<typename F>
inline bool array<T>::each(F&& func) {
  if constexpr (is_same<decltype(func({})), bool>) {
    for (rx_size i{0}; i < m_size; i++) {
      if (!func(operator[](i))) {
        return false;
      }
    }
  } else {
    for (rx_size i{0}; i < m_size; i++) {
      func(operator[](i));
    }
  }
  return true;
}

template<typename T>
template<typename F>
inline bool array<T>::each(F&& func) const {
  return const_cast<array<T>*>(this)->each(func);
}

} // namespace rx

#endif // RX_CORE_ARRAY_H
