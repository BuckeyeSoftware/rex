#ifndef RX_CORE_OPTIONAL_H
#define RX_CORE_OPTIONAL_H
#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/utility/move.h"

#include "rx/core/memory/uninitialized_storage.h" // uninitialized_storage

namespace rx {

namespace detail {
  struct nullopt {};
};

constexpr detail::nullopt nullopt;

template<typename T>
struct optional {
  constexpr optional(detail::nullopt);
  constexpr optional();
  constexpr optional(T&& data);
  constexpr optional(const T& data);
  constexpr optional(optional&& other);
  constexpr optional(const optional& other);

  ~optional();

  optional& operator=(T&& data);
  optional& operator=(const T& data);
  optional& operator=(optional&& other);
  optional& operator=(const optional& other);

  operator bool() const;

  T& operator*();
  const T& operator*() const;
  T* operator->();
  const T* operator->() const;

private:
  memory::uninitialized_storage<T> m_data;
  bool m_init;
};

template<typename T>
inline constexpr optional<T>::optional(decltype(nullopt))
  : m_data{}
  , m_init{false}
{
}

template<typename T>
inline constexpr optional<T>::optional()
  : m_data{}
  , m_init{false}
{
}

template<typename T>
inline constexpr optional<T>::optional(T&& data)
  : m_data{}
  , m_init{true}
{
  m_data.init(utility::move(data));
}

template<typename T>
inline constexpr optional<T>::optional(const T& data)
  : m_data{}
  , m_init{true}
{
  m_data.init(data);
}

template<typename T>
inline constexpr optional<T>::optional(optional&& other)
  : m_data{}
  , m_init{other.m_init}
{
  if (m_init) {
    auto& data{other.m_data};
    m_data.init(utility::move(*data.data()));
    data.fini();
  }
  other.m_init = false;
}

template<typename T>
inline constexpr optional<T>::optional(const optional& other)
  : m_data{}
  , m_init{other.m_init}
{
  if (m_init) {
    const auto& data{other.m_data};
    m_data.init(*data.data());
  }
}

template<typename T>
inline optional<T>& optional<T>::operator=(T&& data) {
  if (m_init) {
    m_data.fini();
  }
  m_init = true;
  m_data.init(utility::move(data));
  return *this;
}

template<typename T>
inline optional<T>& optional<T>::operator=(const T& data) {
  if (m_init) {
    m_data.fini();
  }
  m_init = true;
  m_data.init(data);
  return *this;
}

template<typename T>
inline optional<T>& optional<T>::operator=(optional&& other) {
  RX_ASSERT(&other != this, "self assignment");

  if (m_init) {
    m_data.fini();
  }
  m_init = other.m_init;
  if (m_init) {
    auto& data{other.m_data};
    m_data.init(utility::move(*data.data()));
    data.fini();
  }
  return *this;
}

template<typename T>
inline optional<T>& optional<T>::operator=(const optional& other) {
  RX_ASSERT(&other != this, "self assignment");

  if (m_init) {
    m_data.fini();
  }

  m_init = other.m_init;
  if (m_init) {
    const auto& data{other.m_data};
    m_data.init(*data.data());
  }

  return *this;
}

template<typename T>
inline optional<T>::~optional() {
  if (m_init) {
    m_data.fini();
  }
}

template<typename T>
inline optional<T>::operator bool() const {
  return m_init;
}

template<typename T>
inline T& optional<T>::operator*() {
  RX_ASSERT(m_init, "not valid");
  return *m_data.data();
}

template<typename T>
inline const T& optional<T>::operator*() const {
  RX_ASSERT(m_init, "not valid");
  return *m_data.data();
}

template<typename T>
inline T* optional<T>::operator->() {
  RX_ASSERT(m_init, "not valid");
  return m_data.data();
}

template<typename T>
inline const T* optional<T>::operator->() const {
  RX_ASSERT(m_init, "not valid");
  return m_data.data();
}

} // namespace rx

#endif // RX_CORE_OPTIONAL_H
