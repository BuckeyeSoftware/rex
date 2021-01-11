#ifndef RX_CORE_CONCEPTS_INVOCABLE_H
#define RX_CORE_CONCEPTS_INVOCABLE_H
#include "rx/core/traits/is_same.h"

namespace Rx::Concepts {

// There is a legitimate bug here that I cannot work out.
template<typename F, typename... Ts>
concept Invocable = requires(F&& f, Ts&&... args) {
  Utility::forward<F>(f)(Utility::forward<Ts>(args)...);
};

} // namespace Rx::Concepts

#endif // RX_CORE_CONCEPTS_INVOCABLE_H
