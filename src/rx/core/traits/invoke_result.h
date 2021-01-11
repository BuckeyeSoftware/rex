#ifndef RX_CORE_TRAITS_INVOKE_RESULT_H
#define RX_CORE_TRAITS_INVOKE_RESULT_H
#include "rx/core/utility/invoke.h"
#include "rx/core/utility/declval.h"

namespace Rx::Traits {

template<typename F, typename... Ts>
using InvokeResult =
  decltype(Utility::invoke(Utility::declval<F>(), Utility::declval<Ts>()...));

} // namespace Rx::Traits

#endif // RX_CORE_TRAITS_INVOKE_RESULT_H
