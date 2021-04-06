#ifndef RX_CORE_ABORT_H
#define RX_CORE_ABORT_H
#include "rx/core/format.h"

/// \file abort.h

namespace Rx {

/// \brief Abort with a message
/// \param _message The message.
/// \param _truncated If the message was truncated.
/// \warning This function does not return.
[[noreturn]] RX_API void abort_message(const char* _message, bool _truncated);

/// \brief Abort with a message
/// \param _format The format string.
/// \param _arguments The format arguments.
/// \warning This function does not return.
template<typename... Ts>
[[noreturn]] RX_HINT_FORMAT(1, 0)
void abort(const char* _format, Ts&&... _arguments) {
  // When we have format arguments use an on-stack format buffer.
  if constexpr(sizeof...(Ts) > 0) {
    char buffer[4096];
    const Size length = format_buffer({buffer, sizeof buffer}, _format,
      Utility::forward<Ts>(_arguments)...);
    abort_message(buffer, length >= sizeof buffer);
  } else {
    abort_message(_format, false);
  }
}

} // namespace Rx

#endif // RX_CORE_ABORT_H
