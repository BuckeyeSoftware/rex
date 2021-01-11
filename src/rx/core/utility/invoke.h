#ifndef RX_CORE_UTILITY_INVOKE_H
#define RX_CORE_UTILITY_INVOKE_H
#include "rx/core/utility/forward.h"

namespace Rx::Utility {

template<typename F, typename... Ts>
constexpr decltype(auto) invoke(F&& f, Ts&&... args) {
  return forward<F>(f)(forward<Ts>(args)...);
}

} // namespace Rx::Utility

#endif // RX_CORE_UTILITY_INVOKE_H
