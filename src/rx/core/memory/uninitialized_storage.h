#ifndef RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
#define RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
#include "rx/core/utility/forward.h"
#include "rx/core/utility/nat.h"

#include "rx/core/concepts/no_copy.h" // no_copy
#include "rx/core/concepts/no_move.h" // no_move

#include "rx/core/type_eraser.h" // type_eraser

namespace rx::memory {

// represents uninitialized storage suitable in size and alignment for
// an object of type |T|, can be type erased for implementing deferred static
// globals and variant types
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
    utility::nat m_nat;
    alignas(T) mutable rx_byte m_data[sizeof(T)];
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
  utility::construct<T>(reinterpret_cast<void*>(m_data), utility::forward<Ts>(args)...);
}

template<typename T>
inline void uninitialized_storage<T>::fini() {
  utility::destruct<T>(reinterpret_cast<void*>(m_data));
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
  return {reinterpret_cast<void*>(m_data), traits::type_identity<T>{}, utility::forward<Ts>(args)...};
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
