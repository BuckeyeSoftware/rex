#ifndef RX_CORE_OPTIONAL_H
#define RX_CORE_OPTIONAL_H
#include "rx/core/assert.h" // RX_ASSERT
#include "rx/core/utility/move.h"
#include "rx/core/uninitialized.h"

#include "rx/core/concepts/trivially_destructible.h"
#include "rx/core/concepts/trivially_copyable.h"
#include "rx/core/concepts/copyable.h"

namespace Rx {

constexpr const struct {} nullopt;

template<typename T>
struct Optional {
  constexpr Optional(decltype(nullopt));
  constexpr Optional();
  constexpr Optional(T&& data_);
  constexpr Optional(const T& _data);
  constexpr Optional(Optional&& other_);

  constexpr Optional(const Optional& _other)
    requires Concepts::TriviallyCopyable<T>;

/*
  ~Optional() requires Concepts::TriviallyDestructible<T> = default;
  ~Optional() requires (!Concepts::TriviallyDestructible<T>);
*/
  ~Optional();

  Optional& operator=(T&& data_);
  Optional& operator=(const T& _data);
  Optional& operator=(Optional&& other_);

  Optional& operator=(const Optional& _other)
    requires Concepts::TriviallyCopyable<T>;

  operator bool() const;

  bool has_value() const;

  T& operator*();
  const T& operator*() const;
  T* operator->();
  const T* operator->() const;

private:
  Uninitialized<T> m_data;
  bool m_init;
};

template<typename T>
constexpr Optional<T>::Optional(decltype(nullopt))
  : m_data{}
  , m_init{false}
{
}

template<typename T>
constexpr Optional<T>::Optional()
  : m_data{}
  , m_init{false}
{
}

template<typename T>
constexpr Optional<T>::Optional(T&& data_)
  : m_data{}
  , m_init{true}
{
  m_data.init(Utility::forward<T>(data_));
}


template<typename T>
constexpr Optional<T>::Optional(const T& _data)
  : m_data{}
  , m_init{true}
{
  if constexpr (Concepts::Copyable<T>) {
    if (auto copy = T::copy(_data)) {
      m_data.init(Utility::move(*copy));
    } else {
      m_init = false;
    }
  } else {
    m_data.init(_data);
  }
}

template<typename T>
constexpr Optional<T>::Optional(Optional&& other_)
  : m_data{}
  , m_init{other_.m_init}
{
  if (m_init) {
    auto& data{other_.m_data};
    m_data.init(Utility::move(*data.data()));
    data.fini();
  }
  other_.m_init = false;
}

template<typename T>
constexpr Optional<T>::Optional(const Optional& _other)
  requires Concepts::TriviallyCopyable<T>
  : m_data{}
  , m_init{_other.m_init}
{
  if (m_init) {
    const auto& data{_other.m_data};
    m_data.init(*data.data());
  }
}

template<typename T>
Optional<T>& Optional<T>::operator=(T&& data_) {
  if (m_init) {
    m_data.fini();
  }
  m_init = true;
  m_data.init(Utility::forward<T>(data_));
  return *this;
}

template<typename T>
Optional<T>& Optional<T>::operator=(const T& _data) {
  if (m_init) {
    m_data.fini();
  }
  m_init = true;
  m_data.init(_data);
  return *this;
}

template<typename T>
Optional<T>& Optional<T>::operator=(Optional&& other_) {
  if (&other_ != this) {
    if (m_init) {
      m_data.fini();
    }
    m_init = other_.m_init;
    if (m_init) {
      auto& data = other_.m_data;
      m_data.init(Utility::move(*data.data()));
      data.fini();
    }
    other_.m_init = false;
  }
  return *this;
}

template<typename T>
Optional<T>& Optional<T>::operator=(const Optional& _other)
  requires Concepts::TriviallyCopyable<T>
{
  if (&_other != this) {
    if (m_init) {
      m_data.fini();
    }
    m_init = _other.m_init;
    if (m_init) {
      const auto& data = _other.m_data;
      m_data.init(*data.data());
    }
  }
  return *this;
}

/*
template<typename T>
Optional<T>::~Optional() requires (!Concepts::TriviallyDestructible<T>) {
  if (m_init) {
    m_data.fini();
  }
}*/

template<typename T>
Optional<T>::~Optional() {
  if (m_init) {
    m_data.fini();
  }
}

template<typename T>
Optional<T>::operator bool() const {
  return m_init;
}

template<typename T>
bool Optional<T>::has_value() const {
  return m_init;
}

template<typename T>
T& Optional<T>::operator*() {
  RX_ASSERT(m_init, "not valid");
  return *m_data.data();
}

template<typename T>
const T& Optional<T>::operator*() const {
  RX_ASSERT(m_init, "not valid");
  return *m_data.data();
}

template<typename T>
T* Optional<T>::operator->() {
  RX_ASSERT(m_init, "not valid");
  return m_data.data();
}

template<typename T>
const T* Optional<T>::operator->() const {
  RX_ASSERT(m_init, "not valid");
  return m_data.data();
}

} // namespace Rx

#endif // RX_CORE_OPTIONAL_H
