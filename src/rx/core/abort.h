#ifndef RX_CORE_ABORT_H
#define RX_CORE_ABORT_H
#include "rx/core/format.h"

namespace Rx {

[[noreturn]]
void abort_full(const char* _message);

[[noreturn]]
void abort_truncated(const char* _message);

template<typename... Ts>
[[noreturn]]
void abort(const char* _format, Ts&&... _arguments) {
  // When we have format arguments use an on-stack format buffer.
  if constexpr(sizeof...(Ts) > 0) {
    char buffer[4096];
    const Size result = format_buffer(buffer, sizeof buffer, _format,
      Utility::forward<Ts>(_arguments)...);
    if (result >= sizeof buffer) {
      abort_truncated(buffer);
    } else {
      abort_full(buffer);
    }
  } else {
    abort_full(_format);
  }
}

} // namespace rx

#endif // RX_CORE_ABORT_H
