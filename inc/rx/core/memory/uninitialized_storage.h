#ifndef RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
#define RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H

#include <rx/core/traits.h> // nat, call_{ctor, dtor}
#include <rx/core/type_eraser.h> // type_eraser
#include <rx/core/concepts/no_copy.h> // no_copy
#include <rx/core/concepts/no_move.h> // no_move

namespace rx::memory {

// represents uninitialized storage suitable in size and alignment for
// an object of type |T|
template<typename T>
struct uninitialized_storage
  : concepts::no_copy
  , concepts::no_move
{
  constexpr uninitialized_storage();

  // explicitly initialize the storage with |args|
  template<typename... Ts>
  void init(Ts&&... args);

  // explicitly finalize the storage
  void fini();

  // get the storage
  T* data();
  const T* data() const;

  // type erase the uninitialized storage and capture |args| for constructor
  template<typename... Ts>
  type_eraser type_erase(Ts&&... args) const;

private:
  union {
    nat m_nat;
    alignas(T) rx_byte m_data[sizeof(T)];
  };
};

// uninitialized_storage
template<typename T>
inline constexpr uninitialized_storage<T>::uninitialized_storage()
  : m_nat{}
{
}

template<typename T>
template<typename... Ts>
inline void uninitialized_storage<T>::init(Ts&&... args) {
  call_ctor<T>(reinterpret_cast<void*>(m_data), forward<Ts>(args)...);
}

template<typename T>
inline void uninitialized_storage<T>::fini() {
  call_dtor<T>(reinterpret_cast<void*>(m_data));
}

template<typename T>
inline T* uninitialized_storage<T>::data() {
  return reinterpret_cast<T*>(m_data);
}

template<typename T>
inline const T* uninitialized_storage<T>::data() const {
  return reinterpret_cast<const T*>(m_data);
}

template<typename T>
template<typename... Ts>
inline type_eraser uninitialized_storage<T>::type_erase(Ts&&... args) const {
  return {const_cast<void*>(reinterpret_cast<const void*>(data())), identity<T>{}, forward<Ts>(args)...};
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
