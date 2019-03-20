#ifndef RX_CORE_TUPLE_H
#define RX_CORE_TUPLE_H

#include <rx/core/utility/move.h>
#include <rx/core/utility/forward.h>
#include <rx/core/utility/index_sequence.h>
#include <rx/core/utility/make_index_sequence.h>

namespace rx {

namespace detail {
  template<rx_size N, typename T>
  struct tuple_element {
    constexpr tuple_element(T&& element) : m_element{utility::move(element)} {}
    constexpr T& get() { return m_element; }
    constexpr const T& get() const { return m_element; }
  private:
    T m_element;
  };

  template<typename T, typename... Ts>
  class tuple;
  template<rx_size... Ns, typename... Ts>
  class tuple<utility::index_sequence<Ns...>, Ts...> : public tuple_element<Ns, Ts>... {
    template<rx_size, typename...>
    struct pick_type;

    template<rx_size N, typename P, typename... As>
    struct pick_type<N, P, As...> : pick_type<N - 1, As...> {};
    template<typename P, typename... As>
    struct pick_type<0, P, As...> : traits::type_identity<P> {};

    template<rx_size N> using pick = typename pick_type<N, Ts...>::type;
    template<rx_size N> using elem = tuple_element<N, pick<N>>;
  public:
    constexpr tuple(Ts&&... args) : tuple_element<Ns, Ts>{utility::forward<Ts>(args)}... {}
    template<rx_size N> constexpr pick<N>& get() { return elem<N>::get(); }
    template<rx_size N> constexpr const pick<N>& get() const { return elem<N>::get(); }
  };
} // detail

template<typename... Ts>
struct tuple : detail::tuple<utility::index_sequence_for<Ts...>, Ts...> {
  constexpr tuple(Ts&&... args);
  static constexpr rx_size size();
};

template<typename... Ts>
inline constexpr tuple<Ts...>::tuple(Ts&&... args)
  : detail::tuple<utility::index_sequence_for<Ts...>, Ts...>{utility::forward<Ts>(args)...}
{
}

template<typename... Ts>
inline constexpr rx_size tuple<Ts...>::size() {
  return sizeof...(Ts);
}

} // namespace rx

#endif // RX_CORE_TUPLE_H
