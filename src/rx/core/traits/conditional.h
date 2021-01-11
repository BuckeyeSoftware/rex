#ifndef RX_CORE_TRAITS_CONDITIONAL_H
#define RX_CORE_TRAITS_CONDITIONAL_H

namespace Rx::Traits {

namespace _ {
  template<bool B, typename T, typename F>
  struct Conditional { using Type = T; };
  template<typename T, typename F>
  struct Conditional<false, T, F> { using Type = F; };
}

template<bool B, typename T, typename F>
using Conditional = typename _::Conditional<B, T, F>::Type;

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_CONDITIONAL_H
