#ifndef RX_CORE_TRAITS_H
#define RX_CORE_TRAITS_H

#include <rx/core/types.h> // rx_size

// placement new
inline void* operator new(rx_size, void* data) noexcept {
  return data;
}

namespace rx {

// empty type that is "not-a-type"
struct nat {};

// struct instance that holds identity |type| of |T|
template<typename T>
struct identity {
  using type = T;
};

// remove_reference
namespace detail {
  template<typename T>
  struct remove_reference : identity<T> {};
  template<typename T>
  struct remove_reference<T&> : identity<T> {};
  template<typename T>
  struct remove_reference<T&&> : identity<T> {};
}
template<typename T>
using remove_reference = typename detail::remove_reference<T>::type;

// check if |T1| and |T2| are same types
template<typename T1, typename T2>
inline constexpr bool is_same{false};
template<typename T>
inline constexpr bool is_same<T, T>{true};

// check if |T| is void
template<typename T>
inline constexpr bool is_void{is_same<T, void>};

// check if |T| is a reference type
template<typename T>
inline constexpr bool is_reference{false};
template<typename T>
inline constexpr bool is_reference<T&>{true};
template<typename T>
inline constexpr bool is_reference<T&&>{true};

// add_rvalue_reference
namespace detail {
  template<typename T, bool b>
  struct add_rvalue_reference : identity<T> {};
  template<typename T>
  struct add_rvalue_reference<T, true> : identity<T&&> {};
}
template<typename T>
using add_rvalue_reference =
  typename detail::add_rvalue_reference<T, !is_void<T> && !is_reference<T>>::type;

// declval
template<typename T>
add_rvalue_reference<T> declval() noexcept;

// forward |T| for perfect-forwarding
template<typename T>
inline T&& forward(remove_reference<T>& data) {
  return static_cast<T&&>(data);
}
template<typename T>
inline T&& forward(remove_reference<T>&& data) {
  return static_cast<T&&>(data);
}

// move |T| for move semantics
template<typename T>
inline remove_reference<T>&& move(T&& data) {
  return static_cast<remove_reference<T>&&>(data);
}

// check if |T| is trivially copyable (can memcpy)
template<typename T>
inline constexpr bool is_trivially_copyable = __is_trivially_copyable(T);

// check if |T| is trivially destructible (don't need to invoke a destructor)
template<typename T>
inline constexpr bool is_trivially_destructible = __has_trivial_destructor(T);

// utility functions that call type constructors and destructors
template<typename T, typename... Ts>
inline void call_ctor(void *data, Ts&&... args) {
  new (reinterpret_cast<T*>(data)) T(forward<Ts>(args)...);
}

template<typename T>
inline void call_dtor(void *data) {
  reinterpret_cast<T*>(data)->~T();
}

// conditional
namespace detail {
  template<bool B, typename T, typename F>
  struct conditional : identity<T> {};
  template<typename T, typename F>
  struct conditional<false, T, F> : identity<F> {};
}

template<bool B, typename T, typename F>
using conditional = typename detail::conditional<B, T, F>::type;

} // namespace rx

#endif // RX_CORE_TRAITS_H
