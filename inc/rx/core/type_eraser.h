#ifndef RX_CORE_TYPE_ERASER_H
#define RX_CORE_TYPE_ERASER_H

#include <rx/core/traits.h> // call_{ctor, dtor}
#include <rx/core/tuple.h> // tuple
#include <rx/core/concepts/no_copy.h> // no_copy

namespace rx {

// type erasing variant
// 32-bit: 16 bytes + k_memory
// 64-bit: 32 bytes + k_memory
struct type_eraser : concepts::no_copy {
  static constexpr const rx_size k_alignment{16};
  static constexpr const rx_size k_memory{64};

  // initialize type eraser instance with |data|
  template<typename T, typename... Ts>
  constexpr type_eraser(void *data, identity<T>, Ts&&... args);

  // move a type erased instance
  constexpr type_eraser(type_eraser&& eraser);

  // call the constructor on the type-erased object
  void init();

  // call the destructor on the type-erased object
  void fini();

private:
  void* m_data;
  void (*m_construct_fn)(void*, void*);
  void (*m_destruct_fn)(void*);
  void (*m_move_tuple_fn)(void*, const void*);

  union {
    nat m_nat;
    alignas(k_alignment) rx_byte m_tuple[k_memory];
  };

  template<typename T, typename... Ts, typename U, rx_size... Ns>
  static void construct_with_tuple(void* object_data, [[maybe_unused]] U* tuple_object, sizes<Ns...>) {
    call_ctor<T>(object_data, forward<Ts>(tuple_object->template get<Ns>())...);
  }

  template<typename T, typename... Ts>
  static void construct(void* object_data, void* tuple_data) {
    construct_with_tuple<T, Ts...>(object_data, static_cast<tuple<Ts...>*>(tuple_data),
      index_sequence<sizeof...(Ts)>{});
  }

  template<typename T>
  static void destruct(void* object_data) {
    call_dtor<T>(object_data);
  }

  template<typename T, typename... Ts>
  static void move_tuple(void* tuple_dst, const void* tuple_src) {
    call_ctor<rx::tuple<Ts...>>(tuple_dst, move(*static_cast<const rx::tuple<Ts...>*>(tuple_src)));
  }
};

template<typename T, typename... Ts>
inline constexpr type_eraser::type_eraser(void *data, identity<T>, Ts&&... args)
  : m_data{data}
  , m_construct_fn{construct<T, Ts...>}
  , m_destruct_fn{destruct<T>}
  , m_move_tuple_fn{move_tuple<T, Ts...>}
  , m_nat{}
{
  static_assert(sizeof(rx::tuple<Ts...>) <= sizeof m_tuple, "too much data to type erase");

  call_ctor<rx::tuple<Ts...>>(static_cast<void*>(m_tuple), forward<Ts>(args)...);
}

inline constexpr type_eraser::type_eraser(type_eraser&& eraser)
  : m_data{eraser.m_data}
  , m_construct_fn{eraser.m_construct_fn}
  , m_destruct_fn{eraser.m_destruct_fn}
  , m_move_tuple_fn{eraser.m_move_tuple_fn}
  , m_nat{}
{
  eraser.m_data = nullptr;
  eraser.m_construct_fn = nullptr;
  eraser.m_destruct_fn = nullptr;
  eraser.m_move_tuple_fn = nullptr;

  m_move_tuple_fn(static_cast<void*>(m_tuple), static_cast<const void*>(eraser.m_tuple));
}

inline void type_eraser::init() {
  m_construct_fn(m_data, static_cast<void*>(m_tuple));
}

inline void type_eraser::fini() {
  m_destruct_fn(m_data);
}

} // namespace rx

#endif // RX_CORE_TYPE_ERASER_H
