#ifndef RX_CORE_PTR_H
#define RX_CORE_PTR_H
#include "rx/core/memory/allocator.h"
#include "rx/core/hints/empty_bases.h"
#include "rx/core/utility/exchange.h"
#include "rx/core/hash.h"

namespace rx {

// # Unique pointer
//
// Owning smart-pointer type that releases the data when the object goes out of
// scope. Move-only type.
//
// Since all allocations in Rex are associated with a given allocator, this must
// be given the allocator that allocated the pointer to take ownership of it.
//
// You may use the make_ptr helper to construct a ptr.
//
// There is no support for a custom deleter.
// There is no support for array types, use ptr<array<T[E]>> instead.
//
// 32-bit: 8 bytes
// 64-bit: 16 bytes
template<typename T>
struct RX_HINT_EMPTY_BASES ptr
  : concepts::no_copy
{
  constexpr ptr();
  constexpr ptr(memory::allocator* _allocator);
  constexpr ptr(memory::allocator* _allocator, rx_nullptr);

  template<typename U>
  constexpr ptr(memory::allocator* _allocator, U* _data);

  template<typename U>
  ptr(ptr<U>&& other_);

  ~ptr();

  template<typename U>
  ptr& operator=(ptr<U>&& other_);

  ptr& operator=(rx_nullptr);

  template<typename U>
  void reset(memory::allocator* _allocator, U* _data);

  T* release();

  T& operator*() const;
  T* operator->() const;

  operator bool() const;
  T* get() const;
  memory::allocator* allocator() const;

private:
  void destroy();

  template<typename U>
  friend struct ptr;

  memory::allocator* m_allocator;
  T* m_data;
};

template<typename T>
inline constexpr ptr<T>::ptr()
  : ptr{nullptr}
{
}

template<typename T>
inline constexpr ptr<T>::ptr(memory::allocator* _allocator)
  : m_allocator{_allocator}
  , m_data{nullptr}
{
}

template<typename T>
inline constexpr ptr<T>::ptr(memory::allocator* _allocator, rx_nullptr)
  : ptr{_allocator}
{
}

template<typename T>
template<typename U>
inline constexpr ptr<T>::ptr(memory::allocator* _allocator, U* _data)
  : m_allocator{_allocator}
  , m_data{_data}
{
}

template<typename T>
template<typename U>
inline ptr<T>::ptr(ptr<U>&& other_)
  : m_allocator{utility::exchange(other_.m_allocator, nullptr)}
  , m_data{utility::exchange(other_.m_data, nullptr)}
{
}

template<typename T>
inline ptr<T>::~ptr() {
  destroy();
}

template<typename T>
template<typename U>
inline ptr<T>& ptr<T>::operator=(ptr<U>&& ptr_) {
  // The casts are necessary here since T and U may not be the same.
  RX_ASSERT(reinterpret_cast<rx_uintptr>(&ptr_)
    != reinterpret_cast<rx_uintptr>(this), "self assignment");
  destroy();
  m_allocator = utility::exchange(ptr_.m_allocator, nullptr);
  m_data = utility::exchange(ptr_.m_data, nullptr);
  return *this;
}

template<typename T>
inline ptr<T>& ptr<T>::operator=(rx_nullptr) {
  destroy();
  m_data = nullptr;
  return *this;
}

template<typename T>
template<typename U>
inline void ptr<T>::reset(memory::allocator* _allocator, U* _data) {
  RX_ASSERT(_allocator, "null allocator");
  destroy();
  m_allocator = _allocator;
  m_data = _data;
}

template<typename T>
inline T* ptr<T>::release() {
  return utility::exchange(m_data, nullptr);
}

template<typename T>
inline T& ptr<T>::operator*() const {
  RX_ASSERT(m_data, "nullptr");
  return *m_data;
}

template<typename T>
inline T* ptr<T>::operator->() const {
  return m_data;
}

template<typename T>
inline ptr<T>::operator bool() const {
  return m_data != nullptr;
}

template<typename T>
inline T* ptr<T>::get() const {
  return m_data;
}

template<typename T>
inline memory::allocator* ptr<T>::allocator() const {
  return m_allocator;
}

template<typename T>
inline void ptr<T>::destroy() {
  if (m_data) {
    m_allocator->destroy<T>(m_data);
  }
}

// Helper function to make a unique ptr.
template<typename T, typename... Ts>
inline ptr<T> make_ptr(memory::allocator* _allocator, Ts&&... _arguments) {
  RX_ASSERT(_allocator, "null allocator");
  return {_allocator, _allocator->create<T>(utility::forward<Ts>(_arguments)...)};
}

// Specialization of hash<Ht> for ptr<Pt>.
template<typename T>
struct hash<ptr<T>> {
  constexpr rx_size operator()(const ptr<T>& _ptr) const {
    return hash<T*>{}(_ptr.get());
  }
};

} // namespace rx

#endif // RX_CORE_PTR_H
