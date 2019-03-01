#ifndef RX_CORE_TUPLE_H
#define RX_CORE_TUPLE_H

#include <rx/core/traits.h> // sizes, nth_from_pack, index_sequence

namespace rx {

namespace detail {
  template<rx_size N, typename T>
  struct tuple_element {
    constexpr tuple_element(T&& element) : m_element{move(element)} {}
    constexpr T& get() { return m_element; }
    constexpr const T& get() const { return m_element; }
  private:
    T m_element;
  };

  template<typename T, typename... Ts>
  class tuple;
  template<rx_size... Ns, typename... Ts>
  class tuple<sizes<Ns...>, Ts...> : public tuple_element<Ns, Ts>... {
    template<rx_size N> using pick = ::rx::nth_from_pack<N, Ts...>;
    template<rx_size N> using elem = tuple_element<N, pick<N>>;
  public:
    constexpr tuple(Ts&&... args) : tuple_element<Ns, Ts>{forward<Ts>(args)}... {}
    template<rx_size N> constexpr pick<N>& get() { return elem<N>::get(); }
    template<rx_size N> constexpr const pick<N>& get() const { return elem<N>::get(); }
  };
} // detail

template<typename... Ts>
struct tuple : detail::tuple<index_sequence<sizeof...(Ts)>, Ts...> {
  constexpr tuple(Ts&&... args);
  static constexpr rx_size size();
};

template<typename... Ts>
inline constexpr tuple<Ts...>::tuple(Ts&&... args)
  : detail::tuple<index_sequence<sizeof...(Ts)>, Ts...>{forward<Ts>(args)...}
{
  // {empty}
}

template<typename... Ts>
inline constexpr rx_size tuple<Ts...>::size() {
  return sizeof...(Ts);
}

} // namespace rx

#endif // RX_CORE_TUPLE_H
